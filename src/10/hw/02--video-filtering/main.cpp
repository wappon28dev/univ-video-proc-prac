#include <iostream>
#include <opencv2/opencv.hpp>

#include "../../utils/opencv.hpp"

// 長い型のエイリアス定義
using ComplexVal = cv::Vec2d;

/**
 * @brief 配列の象限を入れ替える関数
 * 
 * 低周波成分を中央、高周波成分を周辺に配置するため（あるいはその逆）に、
 * 第1象限と第3象限、第2象限と第4象限を入れ替える。
 */
void shiftDFT(cv::Mat& mat)
{
    int cx = mat.cols / 2;
    int cy = mat.rows / 2;

    cv::Mat q1(mat, cv::Rect(cx, 0, cx, cy)); // 右上
    cv::Mat q2(mat, cv::Rect(0, 0, cx, cy));  // 左上
    cv::Mat q3(mat, cv::Rect(0, cy, cx, cy)); // 左下
    cv::Mat q4(mat, cv::Rect(cx, cy, cx, cy)); // 右下

    cv::Mat tmp;
    q1.copyTo(tmp);
    q3.copyTo(q1);
    tmp.copyTo(q3);

    q2.copyTo(tmp);
    q4.copyTo(q2);
    tmp.copyTo(q4);
}

/**
 * @brief ハイパスフィルタを適用する関数
 * 
 * cv::Mat::forEach を用いて並列に走査し、
 * 画像中心（低周波成分）から一定距離（半径）以内の領域をゼロにする。
 * 
 * @param ftMatrix 周波数ドメインの複素行列
 * @param radius 遮断周波数（半径）
 */
void applyHighpassFilter(cv::Mat& ftMatrix, double radius)
{
    int cx = ftMatrix.cols / 2;
    int cy = ftMatrix.rows / 2;
    double radiusSq = radius * radius;

    ftMatrix.forEach<ComplexVal>([cx, cy, radiusSq](ComplexVal& pixel, const int* position) {
        int y = position[0];
        int x = position[1];
        double dx = x - cx;
        double dy = y - cy;
        // 画像中心からの距離の二乗が閾値以下のとき、低周波成分として除去する
        if (dx * dx + dy * dy <= radiusSq)
        {
            pixel = ComplexVal(0.0, 0.0);
        }
    });
}

/**
 * @brief 実数画像からフーリエ変換用の複素行列を生成する
 */
cv::Mat createComplexMatrix(const cv::Mat& grayImg)
{
    cv::Mat planes[] = {cv::Mat_<double>(grayImg), cv::Mat::zeros(grayImg.size(), CV_64FC1)};
    cv::Mat complexMat;
    cv::merge(planes, 2, complexMat);
    return complexMat;
}

/**
 * @brief フーリエ逆変換を行い、結果をCV_8UC1の画像として返す
 */
cv::Mat inverseDFT(const cv::Mat& ftMatrix)
{
    cv::Mat complexMat;
    cv::idft(ftMatrix, complexMat);
    
    cv::Mat planes[2];
    cv::split(complexMat, planes);
    
    double minVal, maxVal;
    cv::minMaxLoc(planes[0], &minVal, &maxVal);
    
    cv::Mat resultImg;
    if (maxVal > 0)
    {
        resultImg = cv::Mat(planes[0] * (1.0 / maxVal));
    }
    else
    {
        resultImg = cv::Mat::zeros(planes[0].size(), CV_64FC1);
    }
    resultImg.convertTo(resultImg, CV_8U, 255);
    return resultImg;
}

int main(int argc, char* argv[])
{
    // ビデオキャプチャの初期化
    auto cap = cv::VideoCapture();
    if (argc > 1)
    {
        cap.open(argv[1]);
        std::cout << "Opening video file: " << argv[1] << std::endl;
    }
    else
    {
        cap.open(0);
        std::cout << "Opening camera: 0" << std::endl;
    }

    if (!cap.isOpened())
    {
        std::cerr << "Failed to open video source." << std::endl;
        return -1;
    }

    auto width = static_cast<int>(cap.get(cv::CAP_PROP_FRAME_WIDTH));
    auto height = static_cast<int>(cap.get(cv::CAP_PROP_FRAME_HEIGHT));
    auto fps = cap.get(cv::CAP_PROP_FPS);
    if (fps <= 0)
    {
        fps = 30.0;
    }

    std::cout << "Resolution: " << width << "x" << height << ", FPS: " << fps << std::endl;

    // ビデオライターの初期化（10秒間の動画を out/ ディレクトリに書き出す）
    auto fourcc = cv::VideoWriter::fourcc('m', 'p', '4', 'v');
    auto outPath = "./out/out_hanga.mp4";
    auto writer = cv::VideoWriter(outPath, fourcc, fps, cv::Size(width, height), true);
    if (!writer.isOpened())
    {
        std::cerr << "Failed to open VideoWriter." << std::endl;
        return -1;
    }

    auto frameCount = 0;
    auto maxFrames = static_cast<int>(fps * 10.0); // 10秒分のフレーム数

    auto frame = cv::Mat();
    auto grayFrame = cv::Mat();
    auto resultFrame = cv::Mat();
    auto binaryFrame = cv::Mat();
    auto colorFrame = cv::Mat();

    std::cout << "Processing video. Output will be saved to: " << outPath << std::endl;
    std::cout << "Press 'q' in the window to stop early." << std::endl;

    while (frameCount < maxFrames)
    {
        cap >> frame;
        if (frame.empty())
        {
            std::cout << "End of video stream." << std::endl;
            break;
        }

        // 1. グレースケール変換
        cv::cvtColor(frame, grayFrame, cv::COLOR_BGR2GRAY);

        // 2. DFT実行
        auto cxMatrix = createComplexMatrix(grayFrame);
        auto ftMatrix = cv::Mat();
        cv::dft(cxMatrix, ftMatrix);
        shiftDFT(ftMatrix);

        // 3. ハイパスフィルタの適用 (forEach 化)
        applyHighpassFilter(ftMatrix, 15.0);

        // 4. IDFT実行
        shiftDFT(ftMatrix);
        resultFrame = inverseDFT(ftMatrix);

        // 5. 2値化処理（版画風に変換）
        cv::threshold(resultFrame, binaryFrame, 30, 255, cv::THRESH_BINARY);

        // 6. カラー(3チャンネル)に変換して動画に書き込み
        cv::cvtColor(binaryFrame, colorFrame, cv::COLOR_GRAY2BGR);
        writer << colorFrame;

        // mat_util::show を使用してリサイズ表示
        mat_util::show("Original", frame);
        mat_util::show("Hanga-like", binaryFrame);

        frameCount++;

        if (cv::waitKey(1) == 'q')
        {
            std::cout << "Interrupted by user." << std::endl;
            break;
        }
    }

    std::cout << "Completed. Processed " << frameCount << " frames." << std::endl;
    return 0;
}

#include <iostream>
#include <opencv2/opencv.hpp>
#include <cmath>

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
 * @brief ローパスフィルタを適用する関数
 * 
 * cv::Mat::forEach を用いて並列に走査し、
 * 画像中心（低周波）から一定距離（半径）以上離れた画素（高周波成分）をゼロにする。
 * 
 * @param ftMatrix 周波数ドメインの複素行列
 * @param radius 遮断周波数（半径）
 */
void applyLowpassFilter(cv::Mat& ftMatrix, double radius)
{
    int cx = ftMatrix.cols / 2;
    int cy = ftMatrix.rows / 2;
    double radiusSq = radius * radius;

    ftMatrix.forEach<ComplexVal>([cx, cy, radiusSq](ComplexVal& pixel, const int* position) {
        int y = position[0];
        int x = position[1];
        double dx = x - cx;
        double dy = y - cy;
        // 距離の二乗が閾値を超えたら高周波成分とみなして除去する
        if (dx * dx + dy * dy > radiusSq)
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
 * @brief 複素周波数行列から表示用のスペクトル画像を生成する
 */
cv::Mat calculateSpectrumImage(const cv::Mat& ftMatrix)
{
    cv::Mat planes[2];
    cv::split(ftMatrix, planes);
    
    cv::Mat magnitudeMat;
    cv::magnitude(planes[0], planes[1], magnitudeMat);
    
    magnitudeMat += cv::Scalar::all(1);
    cv::log(magnitudeMat, magnitudeMat);
    
    cv::Mat spectrumImg;
    cv::normalize(magnitudeMat, spectrumImg, 0, 255, cv::NORM_MINMAX, CV_8U);
    return spectrumImg;
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
    auto inputPath = "./assets/nybuildings.jpg";
    auto sourceImg = cv::imread(inputPath, cv::IMREAD_GRAYSCALE);
    if (sourceImg.empty())
    {
        std::cerr << "Failed to load image: " << inputPath << std::endl;
        return -1;
    }

    // 複素行列の作成と離散フーリエ変換 (DFT)
    auto cxMatrix = createComplexMatrix(sourceImg);
    auto ftMatrix = cv::Mat();
    cv::dft(cxMatrix, ftMatrix);
    
    // 低周波成分を中央に集める
    shiftDFT(ftMatrix);

    // 課題：ローパスフィルタの適用 (中心から半径30ピクセル以内の低周波成分のみ残す)
    auto filterRadius = 30.0;
    applyLowpassFilter(ftMatrix, filterRadius);

    // 表示用のスペクトル画像を生成
    auto spcImg = calculateSpectrumImage(ftMatrix);

    // 逆変換の前に象限を元に戻す
    shiftDFT(ftMatrix);
    
    // フーリエ逆変換 (IDFT) の実行
    auto resultImg = inverseDFT(ftMatrix);

    // 結果の保存 (out/ ディレクトリに配置)
    cv::imwrite("./out/mono.jpg", sourceImg);
    cv::imwrite("./out/out_spectrum.jpg", spcImg);
    cv::imwrite("./out/out_result.jpg", resultImg);

    // ウィンドウ表示 (mat_util::show を使用してリサイズ)
    mat_util::show("Source Image", sourceImg);
    mat_util::show("Spectrum Image", spcImg);
    mat_util::show("Result Image", resultImg);

    std::cout << "Press any key in the GUI windows to exit." << std::endl;
    cv::waitKey(0);
    
    return 0;
}

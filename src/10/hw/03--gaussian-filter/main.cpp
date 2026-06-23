#include <cmath>
#include <iostream>
#include <opencv2/opencv.hpp>

#include "../../utils/opencv.hpp"

// 長い型のエイリアス定義
using ComplexVal = cv::Vec2d;

// フィルタタイプの列挙型
enum class FilterType
{
  Lowpass,
  Highpass,
  Bandpass
};

/**
 * @brief 配列の象限を入れ替える関数
 */
void shiftDFT(cv::Mat &mat)
{
  int cx = mat.cols / 2;
  int cy = mat.rows / 2;

  cv::Mat q1(mat, cv::Rect(cx, 0, cx, cy));  // 右上
  cv::Mat q2(mat, cv::Rect(0, 0, cx, cy));   // 左上
  cv::Mat q3(mat, cv::Rect(0, cy, cx, cy));  // 左下
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
 * @brief ガウス型フィルタを適用する関数
 *
 * cv::Mat::forEach を用いて並列に走査し、
 * 周波数ドメインにおいて、ガウス分布に基づいた重みを各セルに乗算する。
 * u, v は画像中心からの正規化された距離 (最大半径を 1.0 とする) とし、
 * 各フィルタの伝達関数を計算する。
 *
 * @param ftMatrix 周波数ドメインの複素行列
 * @param type フィルタの種類 (Lowpass, Highpass, Bandpass)
 * @param sigma フィルタの広がりパラメータ
 */
void applyGaussianFilter(cv::Mat &ftMatrix, FilterType type, double sigma)
{
  int cx = ftMatrix.cols / 2;
  int cy = ftMatrix.rows / 2;
  double piSq = M_PI * M_PI;

  ftMatrix.forEach<ComplexVal>([cx, cy, piSq, type, sigma](ComplexVal &pixel, const int *position) {
    int y = position[0];
    int x = position[1];

    // 中心を基準とした周波数座標 (正規化なしのピクセル座標)
    double u = x - cx;
    double v = y - cy;
    double distSq = u * u + v * v; // u^2 + v^2

    // ガウス型ローパスの式: G_LP(u, v) = exp(-2 * pi^2 * sigma^2 * (u^2 + v^2))
    double lpf = std::exp(-2.0 * piSq * sigma * sigma * distSq);
    double weight = 0.0;

    switch (type)
    {
    case FilterType::Lowpass:
      weight = lpf;
      break;
    case FilterType::Highpass:
      weight = 1.0 - lpf;
      break;
    case FilterType::Bandpass:
      // ガウス型バンドパス (Laplacian of Gaussian に対応する周波数伝達関数)
      // G_BP(u, v) = -4 * pi^2 * (u^2 + v^2) * exp(-2 * pi^2 * sigma^2 * (u^2 + v^2))
      weight = -4.0 * piSq * distSq * lpf;
      break;
    }

    // 複素行列の各値に重みを掛ける
    pixel *= weight;
  });
}

/**
 * @brief 実数画像からフーリエ変換用の複素行列を生成する
 */
cv::Mat createComplexMatrix(const cv::Mat &grayImg)
{
  cv::Mat planes[] = {cv::Mat_<double>(grayImg), cv::Mat::zeros(grayImg.size(), CV_64FC1)};
  cv::Mat complexMat;
  cv::merge(planes, 2, complexMat);
  return complexMat;
}

/**
 * @brief 複素周波数行列から表示用のスペクトル画像を生成する
 */
cv::Mat calculateSpectrumImage(const cv::Mat &ftMatrix)
{
  cv::Mat planes[2];
  cv::split(ftMatrix, planes);

  cv::Mat magnitudeMat;
  cv::magnitude(planes[0], planes[1], magnitudeMat);

  // 対数スケーリング
  magnitudeMat += cv::Scalar::all(1);
  cv::log(magnitudeMat, magnitudeMat);

  cv::Mat spectrumImg;
  cv::normalize(magnitudeMat, spectrumImg, 0, 255, cv::NORM_MINMAX, CV_8U);
  return spectrumImg;
}

/**
 * @brief フーリエ逆変換を行い、結果をCV_8UC1の画像として返す
 *
 * フィルタ適用後は直流値が変化するため、最小値・最大値を用いて [0, 255] に正規化する。
 */
cv::Mat inverseDFT(const cv::Mat &ftMatrix)
{
  cv::Mat complexMat;
  cv::idft(ftMatrix, complexMat);

  cv::Mat planes[2];
  cv::split(complexMat, planes);

  cv::Mat resultImg;
  // 逆変換結果の実数部の絶対値を取り、0-255 の範囲に正規化する
  cv::Mat absReal = cv::abs(planes[0]);
  cv::normalize(absReal, resultImg, 0, 255, cv::NORM_MINMAX, CV_8U);
  return resultImg;
}

/**
 * @brief 指定されたフィルタを適用し、結果を保存・表示するヘルパー関数
 */
void processAndSave(const cv::Mat &sourceImg, FilterType type, double sigma, const std::string &name,
                    const std::string &outSpecPath, const std::string &outResultPath)
{
  // DFT実行
  auto cxMatrix = createComplexMatrix(sourceImg);
  auto ftMatrix = cv::Mat();
  cv::dft(cxMatrix, ftMatrix);
  shiftDFT(ftMatrix);

  // ガウスフィルタ適用
  applyGaussianFilter(ftMatrix, type, sigma);

  // スペクトル画像生成
  auto spcImg = calculateSpectrumImage(ftMatrix);

  // IDFT実行
  shiftDFT(ftMatrix);
  auto resultImg = inverseDFT(ftMatrix);

  // 画像保存
  cv::imwrite(outSpecPath, spcImg);
  cv::imwrite(outResultPath, resultImg);

  // mat_util::show を使用して表示
  mat_util::show(name + " Spectrum", spcImg);
  mat_util::show(name + " Result", resultImg);
}

int main(int argc, char *argv[])
{
  auto inputPath = "./assets/nymegami.jpg";
  auto sourceImg = cv::imread(inputPath, cv::IMREAD_GRAYSCALE);
  if (sourceImg.empty())
  {
    std::cerr << "Failed to load image: " << inputPath << std::endl;
    return -1;
  }

  mat_util::show("Original Image", sourceImg);

  // 入力のモノクロ画像を out/mono.jpg に出力
  cv::imwrite("./out/mono.jpg", sourceImg);

  std::cout << "Applying Gaussian filters..." << std::endl;

  // 空間標準偏差 sigma = 5.0 に対応する周波数空間での実効シグマを計算 (sigma_freq = 5.0 / width)
  double sigma_lpf_hpf = 5.0 / sourceImg.cols;

  // 1. ガウス型ローパスフィルタ
  processAndSave(sourceImg, FilterType::Lowpass, sigma_lpf_hpf, "Gaussian LPF", "./out/lpf_spectrum.jpg",
                 "./out/lpf_result.jpg");

  // 2. ガウス型ハイパスフィルタ
  processAndSave(sourceImg, FilterType::Highpass, sigma_lpf_hpf, "Gaussian HPF", "./out/hpf_spectrum.jpg",
                 "./out/hpf_result.jpg");

  // 3. ガウス型バンドパスフィルタ (sigma = 0.05, 0.03, 0.01)
  processAndSave(sourceImg, FilterType::Bandpass, 0.05, "Gaussian BPF 0.05", "./out/bpf_005_spectrum.jpg",
                 "./out/bpf_005_result.jpg");
  processAndSave(sourceImg, FilterType::Bandpass, 0.03, "Gaussian BPF 0.03", "./out/bpf_003_spectrum.jpg",
                 "./out/bpf_003_result.jpg");
  processAndSave(sourceImg, FilterType::Bandpass, 0.01, "Gaussian BPF 0.01", "./out/bpf_001_spectrum.jpg",
                 "./out/bpf_001_result.jpg");

  std::cout << "All images saved. Press any key to exit." << std::endl;
  cv::waitKey(0);

  return 0;
}

#include <iostream>
#include <map>
#include <opencv2/opencv.hpp>
#include <string>

#define INPUT_FILENAME "./assets/me@2024-04-11.png"

void show(const std::string &name, const cv::Mat &img)
{
  cv::imshow(name, img);
  cv::namedWindow(name, cv::WINDOW_NORMAL | cv::WINDOW_FREERATIO);
  const auto h = 300;
  const auto w = static_cast<int>(h * img.cols / img.rows);
  cv::resizeWindow(name, w, h);
}

/**
 * 自分のカラー顔写真を原画像として，これをグレースケールに変換した画像，
 * および2値化しきい値処理した画像を生成しなさい．
 * 原画像カラー画像，グレースケール画像，2値化しきい値処理画像をそれぞれウィンドウで表示した状態で
 * スクリーンショットを撮って，1枚の画像として提出しなさい．
 */
int main(int argc, char *argv[], char *envp[])
{

  auto input = cv::imread(INPUT_FILENAME);

  if (input.empty())
  {
    std::cerr << "読み込み失敗！" << std::endl;
    return -1;
  }

  auto grayscale = cv::Mat(input.size(), CV_8UC1);
  cv::cvtColor(input, grayscale, cv::COLOR_BGR2GRAY);

  auto binarized = cv::Mat(input.size(), CV_8UC1);
  cv::threshold(grayscale, binarized, 128, 255, cv::THRESH_BINARY);

  auto imgs_map = std::map<std::string, cv::Mat>{//
                                                 {"input", input},
                                                 {"grayscale", grayscale},
                                                 {"binarized", binarized}};

  for (const auto &[name, img] : imgs_map)
  {
    show(name, img);
  }

  cv::waitKey(0);

  return 0;
}

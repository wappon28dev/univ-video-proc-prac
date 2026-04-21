#include <algorithm>
#include <iostream>
#include <opencv2/opencv.hpp>
#include <string>

struct Context
{
  const cv::Mat *src;
  cv::Mat *gray;
  cv::Mat *dist;
  std::string window_name;

  int *threshold;
  int *channel_idx;

  int *blur_radius;
  int *blur_sigma10;
};

static void on_params_change(int, void *userdata)
{
  auto *ctx = static_cast<Context *>(userdata);

  if (ctx->channel_idx != nullptr)
  {
    cv::extractChannel(*ctx->src, *ctx->gray, *ctx->channel_idx);
  }
  else
  {
    cv::cvtColor(*ctx->src, *ctx->gray, cv::COLOR_BGR2GRAY);
  }

  const int blur_radius = (ctx->blur_radius != nullptr) ? std::max(0, *ctx->blur_radius) : 0;
  if (blur_radius > 0)
  {
    const int ksize = 2 * blur_radius + 1;
    const int sigma10 = (ctx->blur_sigma10 != nullptr) ? std::max(0, *ctx->blur_sigma10) : 0;
    const double sigma = (sigma10 > 0) ? (static_cast<double>(sigma10) / 10.0) : 0.0;

    cv::GaussianBlur(*ctx->gray, *ctx->gray, cv::Size(ksize, ksize), sigma, sigma);
  }

  cv::threshold(*ctx->gray, *ctx->dist, *ctx->threshold, 255, cv::THRESH_BINARY);
  cv::imshow(ctx->window_name, *ctx->dist);
}

#define INPUT_FILENAME "./assets/kadai01-2.jpg"

/**
 * カラー画像ファイル "kadai01-2.jpg"にはある「人物」の顔画像が埋め込まれている．
 * この画像にグレースケール化と2値化しきい値処理を施すことで，この「人物」の顔画像（輪郭線画像）を
 * できるかぎりはっきりと抽出しなさい．
 * 原画像と2値化しきい値処理画像をそれぞれウィンドウで表示した状態でスクリーンショットを撮って，
 * スクリーンショット画像を提出しなさい．
 */
int main(int argc, char *argv[], char *envp[])
{

  auto input = cv::imread(INPUT_FILENAME);

  if (input.empty())
  {
    std::cerr << "読み込み失敗！" << std::endl;
    return -1;
  }

  const auto input_window = "input";
  cv::namedWindow(input_window, cv::WINDOW_NORMAL);
  cv::imshow(input_window, input);
  cv::resizeWindow(input_window, 300, 300);

  const auto dist_win_name = "binarized";
  cv::namedWindow(dist_win_name, cv::WINDOW_NORMAL);

  cv::Mat gray(input.size(), CV_8UC1);
  cv::Mat binarized(input.size(), CV_8UC1);

  int threshold_value = 128;
  int channel_idx = 2;
  int blur_radius = 1;
  int blur_sigma10 = 0;
  auto ctx = Context{
      .src = &input,
      .gray = &gray,
      .dist = &binarized,
      .window_name = dist_win_name,
      .threshold = &threshold_value,
      .channel_idx = &channel_idx,
      .blur_radius = &blur_radius,
      .blur_sigma10 = &blur_sigma10,
  };
  cv::createTrackbar("threshold", dist_win_name, &threshold_value, 255, on_params_change, &ctx);
  cv::createTrackbar("channel(0=B 1=G 2=R)", dist_win_name, &channel_idx, 2, on_params_change, &ctx);
  cv::createTrackbar("blur radius", dist_win_name, &blur_radius, 20, on_params_change, &ctx);
  cv::createTrackbar("sigma x (x0.1)", dist_win_name, &blur_sigma10, 200, on_params_change, &ctx);

  on_params_change(0, &ctx);
  cv::resizeWindow(dist_win_name, 300, 300);

  // press q to quit (trackbars update the result)
  while (true)
  {
    auto key = cv::waitKey(0);
    if (key == 'r')
    {
      on_params_change(0, &ctx);
      cv::imshow(dist_win_name, binarized);
    }
    else if (key == 'q')
    {
      cv::destroyAllWindows();
      break;
    }
  }

  return 0;
}

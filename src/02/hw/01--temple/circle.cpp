#include <iostream>
#include <opencv2/opencv.hpp>

#include "../../utils/opencv.hpp"

#define INPUT_FILENAME "./assets/temple.jpg"
#define RADIUS_RATIO 0.7

int main(int argc, char *argv[], char *envp[])
{

  auto input = cv::imread(INPUT_FILENAME);

  auto [h, w] = std::make_tuple(input.rows, input.cols);
  auto [cx, cy] = std::make_tuple(w / 2, h / 2);

  auto radius = std::sqrt(cx * cx + cy * cy);

  /// 中心からの距離を求める
  auto get_radius = [&](int x, int y) -> double {
    auto dx = x - cx;
    auto dy = y - cy;
    return std::sqrt(dx * dx + dy * dy);
  };

  auto output = input.clone();
  output.forEach<cv::Vec3b>([&](cv::Vec3b &pixel, const int *position) -> void {
    auto stat = mat_util::stat(position, pixel);

    auto distance = get_radius(stat.x, stat.y);
    auto scalar = RADIUS_RATIO - (distance / radius);
    auto [b, g, r] = stat.color;
    pixel = vec3b_util::from(cv::Scalar(b * scalar, g * scalar, r * scalar));
  });

  mat_util::show("input", input);
  mat_util::show("output", output);
  cv::waitKey(0);

  return 0;
}

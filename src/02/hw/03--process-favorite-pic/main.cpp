#include <iostream>
#include <opencv2/opencv.hpp>

#include "../../utils/opencv.hpp"

#define INPUT_FILENAME "./assets/input.png"
#define RADIUS_RATIO 0.9
#define FUON_OFFSET 120

auto is_zunda = [](const vec3b_util::Color &color) -> bool {
  auto [h, s, v] = color;
  return (h >= 30 && h <= 85) && (s >= 50) && (v >= 50);
};

auto sakurize_pixel = [](const vec3b_util::Color &color) -> vec3b_util::Color {
  auto [h, s, v] = color;
  // ref: https://x.com/namigroove/status/1997832396608295088
  return {169, s * 0.4, v};
};

cv::Mat sakurize(const cv::Mat &input)
{

  cv::Mat output_hsv;
  cv::cvtColor(input, output_hsv, cv::COLOR_BGR2HSV);

  output_hsv.forEach<cv::Vec3b>([&](cv::Vec3b &pixel, const int *position) -> void {
    auto stat = mat_util::stat(position, pixel);
    if (is_zunda(stat.color))
    {
      auto [h, s, v] = sakurize_pixel(stat.color);
      pixel = vec3b_util::from(cv::Scalar(h, s, v));
    }
  });

  cv::Mat output;
  cv::cvtColor(output_hsv, output, cv::COLOR_HSV2BGR);
  return output;
};

cv::Mat fuonize(const cv::Mat &input)
{
  auto output = input.clone();

  auto [h, w] = std::make_tuple(input.rows, input.cols);
  auto [cx, cy] = std::make_tuple(w / 2 + FUON_OFFSET, h / 2 - FUON_OFFSET);
  auto radius = std::sqrt(cx * cx + cy * cy);
  /// 中心からの距離を求める
  auto get_radius = [&](int x, int y) -> double {
    auto dx = x - cx;
    auto dy = y - cy;
    return std::sqrt(dx * dx + dy * dy);
  };

  output.forEach<cv::Vec3b>([&](cv::Vec3b &pixel, const int *position) -> void {
    auto stat = mat_util::stat(position, pixel);

    auto distance = get_radius(stat.x, stat.y);
    auto scalar = RADIUS_RATIO - (distance / radius);
    auto [b, g, r] = stat.color;
    pixel = vec3b_util::from(cv::Scalar(b * scalar, g * scalar, r * scalar));
  });

  return output;
};

int main(int argc, char *argv[], char *envp[])
{

  auto zundamon = cv::imread(INPUT_FILENAME);
  auto sakuradamon = sakurize(zundamon);
  auto fuonsakuradamon = fuonize(sakuradamon);

  const auto window_h = 300;
  mat_util::show("zundamon", zundamon, window_h);
  mat_util::show("sakuradamon", sakuradamon, window_h);
  mat_util::show("fuonsakuradamon", fuonsakuradamon, window_h);
  cv::waitKey(0);

  return 0;
}

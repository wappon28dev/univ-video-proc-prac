#include <iostream>
#include <opencv2/opencv.hpp>

#include "../../utils/opencv.hpp"

#define INPUT_FILENAME "./assets/color2.jpg"

int main(int argc, char *argv[], char *envp[])
{

  auto input = cv::imread(INPUT_FILENAME);

  auto is_green = [](const vec3b_util::Color &color) -> bool {
    auto [h, s, v] = color;
    return (h > 70 && h < 85) && (s > 40) && (v > 40);
  };

  cv::Mat output_hsv;
  cv::cvtColor(input, output_hsv, cv::COLOR_BGR2HSV);

  output_hsv.forEach<cv::Vec3b>([&](cv::Vec3b &pixel, const int *position) -> void {
    auto stat = mat_util::stat(position, pixel);

    if (!is_green(stat.color))
    {
      auto [h, s, v] = stat.color;
      pixel = vec3b_util::from(cv::Scalar(h, 0, v));
    }
  });

  auto output = output_hsv.clone();
  cv::cvtColor(output_hsv, output, cv::COLOR_HSV2BGR);

  mat_util::show("input", input);
  mat_util::show("output", output);
  cv::waitKey(0);

  return 0;
}

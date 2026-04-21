#include <iostream>
#include <opencv2/opencv.hpp>

#include "../../utils/opencv.hpp"

#define INPUT_FILENAME1 "./assets/color1.jpg"

vec3b_util::Color invert(vec3b_util::Color color)
{
  auto [b, g, r] = color;
  return {255 - b, 255 - g, 255 - r};
}

auto within_rectangle(int x, int y)
{
  auto x_div = x / 75;
  auto y_div = y / 40;

  auto row_1 = (x_div) % 2 == 0 && (y_div) % 2 == 1;
  auto row_2 = (x_div) % 2 == 1 && (y_div) % 2 == 0;

  return row_1 || row_2;
}

int main(int argc, char *argv[], char *envp[])
{
  auto input = cv::imread(INPUT_FILENAME1);

  auto output = input.clone();
  output.forEach<cv::Vec3b>([](cv::Vec3b &pixel, const int *position) -> void {
    auto stat = mat_util::stat(position, pixel);

    if (within_rectangle(stat.x, stat.y))
    {
      auto [b, g, r] = invert(stat.color);
      pixel = vec3b_util::from(cv::Scalar(b, g, r));
    }
  });

  mat_util::show("input", input);
  mat_util::show("output", output);
  cv::waitKey(0);

  return 0;
}

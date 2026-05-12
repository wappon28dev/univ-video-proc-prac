#include <iostream>
#include <opencv2/opencv.hpp>

namespace vec3b_util
{
cv::Vec3b from(const cv::Scalar &s)
{
  auto [b, g, r] = std::make_tuple(s[0], s[1], s[2]);
  return cv::Vec3b(static_cast<uchar>(b), static_cast<uchar>(g), static_cast<uchar>(r));
}

typedef std::tuple<uchar, uchar, uchar> Color;
Color to_color(const cv::Vec3b &pixel)
{
  auto [b, g, r] = std::make_tuple(pixel[0], pixel[1], pixel[2]);
  return std::make_tuple(b, g, r);
}
} // namespace vec3b_util

namespace mat_util
{
void show(const std::string &name, const cv::Mat &img, int h = 500)
{
  cv::imshow(name, img);
  cv::namedWindow(name, cv::WINDOW_NORMAL | cv::WINDOW_FREERATIO);
  const auto w = static_cast<int>(h * img.cols / img.rows);
  cv::resizeWindow(name, w, h);
}

struct Stat
{
  int x;
  int y;
  vec3b_util::Color color;
};
Stat stat(const int *position, const cv::Vec3b &pixel)
{

  return Stat{position[1], position[0], vec3b_util::to_color(pixel)};
}
} // namespace mat_util

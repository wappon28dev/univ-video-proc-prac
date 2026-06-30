#pragma once

#include <iostream>
#include <opencv2/opencv.hpp>

namespace vec3b_util
{
cv::Vec3b from(const cv::Scalar &s);
typedef std::tuple<uchar, uchar, uchar> Color;
Color to_color(const cv::Vec3b &pixel);
} // namespace vec3b_util

namespace mat_util
{
void show(const std::string &name, const cv::Mat &img, int h = 500);
struct Stat
{
  int x;
  int y;
  vec3b_util::Color color;
};
Stat stat(const int *position, const cv::Vec3b &pixel);
} // namespace mat_util

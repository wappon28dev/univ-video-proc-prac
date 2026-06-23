#include <iostream>
#include <opencv2/opencv.hpp>
#include <ranges>
#include <vector>
#include <numeric>
#include <algorithm>
#include <cmath>

#include "../../utils/opencv.hpp"

#define INPUT_FILENAME "./assets/card.mov"
#define OUTPUT_FILENAME "./out/detected_card.mov"

// Representation of a line in polar coordinates
struct PolarLine
{
  float rho;
  float theta;
};

// Calculate the intersection of two polar lines
bool get_intersection(const PolarLine &l1, const PolarLine &l2, cv::Point &pt)
{
  const double det = std::sin(l2.theta - l1.theta);
  if (std::abs(det) < 1e-5)
  {
    return false;
  }
  const double x = (l1.rho * std::sin(l2.theta) - l2.rho * std::sin(l1.theta)) / det;
  const double y = (l2.rho * std::cos(l1.theta) - l1.rho * std::cos(l2.theta)) / det;
  pt.x = cvRound(x);
  pt.y = cvRound(y);
  return true;
}

// Find the 4 edges of the card using Hough line filtering & NMS
std::vector<PolarLine> find_card_lines(const std::vector<cv::Vec2f> &raw_lines)
{
  auto unique_lines = std::vector<PolarLine>();

  // 1. Non-Maximum Suppression (NMS) in Hough space
  for (const auto &rl : raw_lines)
  {
    const float rho = rl[0];
    const float theta = rl[1];

    bool is_duplicate = false;
    for (const auto &ul : unique_lines)
    {
      double d_theta = std::abs(theta - ul.theta);
      d_theta = std::min(d_theta, 2.0 * M_PI - d_theta);
      const double d_rho = std::abs(rho - ul.rho);

      // If angle difference < 15 deg and rho difference < 40 pixels, it's the same edge
      if (d_theta < (15.0 * M_PI / 180.0) && d_rho < 40.0)
      {
        is_duplicate = true;
        break;
      }
    }
    if (!is_duplicate)
    {
      unique_lines.push_back({rho, theta});
    }
  }

  if (unique_lines.size() < 2)
  {
    return {};
  }

  // 2. Classify lines into Group A (parallel to dominant line) and Group B (perpendicular)
  const auto line0 = unique_lines[0];
  auto card_lines = std::vector<PolarLine>{line0};

  // Find the parallel line (Group A)
  PolarLine parallel_line;
  bool found_parallel = false;
  for (size_t i = 1; i < unique_lines.size(); ++i)
  {
    const auto u = unique_lines[i];
    double d_theta = std::abs(u.theta - line0.theta);
    d_theta = std::min(d_theta, 2.0 * M_PI - d_theta);
    const double d_rho = std::abs(u.rho - line0.rho);

    if (d_theta < (15.0 * M_PI / 180.0) && d_rho > 50.0)
    {
      parallel_line = u;
      found_parallel = true;
      break;
    }
  }
  if (!found_parallel)
  {
    return {};
  }
  card_lines.push_back(parallel_line);

  // Find two perpendicular lines (Group B)
  auto perp_candidates = std::vector<PolarLine>();
  for (size_t i = 1; i < unique_lines.size(); ++i)
  {
    const auto u = unique_lines[i];
    double d_theta = std::abs(u.theta - (line0.theta + M_PI / 2.0));
    d_theta = std::min(d_theta, 2.0 * M_PI - d_theta);
    double d_theta2 = std::abs(u.theta - (line0.theta - M_PI / 2.0));
    d_theta2 = std::min(d_theta2, 2.0 * M_PI - d_theta2);

    if (std::min(d_theta, d_theta2) < (15.0 * M_PI / 180.0))
    {
      bool is_new = true;
      for (const auto &p : perp_candidates)
      {
        if (std::abs(u.rho - p.rho) < 40.0)
        {
          is_new = false;
          break;
        }
      }
      if (is_new)
      {
        perp_candidates.push_back(u);
        if (perp_candidates.size() == 2)
        {
          break;
        }
      }
    }
  }

  if (perp_candidates.size() < 2)
  {
    return {};
  }
  card_lines.push_back(perp_candidates[0]);
  card_lines.push_back(perp_candidates[1]);

  return card_lines;
}

// Sort points in counterclockwise order
std::vector<cv::Point> sort_corners(const std::vector<cv::Point> &pts)
{
  if (pts.size() != 4)
  {
    return pts;
  }

  double sum_x = 0.0;
  double sum_y = 0.0;
  for (const auto &pt : pts)
  {
    sum_x += pt.x;
    sum_y += pt.y;
  }
  const double cx = sum_x / 4.0;
  const double cy = sum_y / 4.0;

  auto sorted = pts;
  std::ranges::sort(sorted, [cx, cy](const cv::Point &a, const cv::Point &b) {
    return std::atan2(a.y - cy, a.x - cx) < std::atan2(b.y - cy, b.x - cx);
  });
  return sorted;
}

int main(int argc, char *argv[])
{
  auto capture = cv::VideoCapture(INPUT_FILENAME);
  if (!capture.isOpened())
  {
    std::cerr << "Failed to open video file: " << INPUT_FILENAME << std::endl;
    return -1;
  }

  const double fps = capture.get(cv::CAP_PROP_FPS);
  const int w = static_cast<int>(capture.get(cv::CAP_PROP_FRAME_WIDTH));
  const int h = static_cast<int>(capture.get(cv::CAP_PROP_FRAME_HEIGHT));
  const auto imageSize = cv::Size(w, h);

  std::cout << "Video loaded: " << INPUT_FILENAME << " (" << w << "x" << h << ", " << fps << " FPS)" << std::endl;

  auto writer = cv::VideoWriter(
      OUTPUT_FILENAME,
      cv::VideoWriter::fourcc('m', 'p', '4', 'v'),
      fps,
      imageSize
  );

  if (!writer.isOpened())
  {
    std::cerr << "Failed to open video writer: " << OUTPUT_FILENAME << std::endl;
    return -1;
  }

  auto originalImage = cv::Mat();
  auto frameImage = cv::Mat(imageSize, CV_8UC3);
  auto grayImage = cv::Mat(imageSize, CV_8UC1);
  auto blurredImage = cv::Mat(imageSize, CV_8UC1);
  auto edgeImage = cv::Mat(imageSize, CV_8UC1);

  auto last_corners = std::vector<cv::Point>();
  int frame_index = 0;

  while (true)
  {
    capture >> originalImage;
    if (originalImage.empty())
    {
      break;
    }

    cv::resize(originalImage, frameImage, imageSize);
    cv::cvtColor(frameImage, grayImage, cv::COLOR_BGR2GRAY);

    // Apply Gaussian Blur to filter out wood grain noise
    cv::GaussianBlur(grayImage, blurredImage, cv::Size(5, 5), 0);

    // Canny edge detection
    cv::Canny(blurredImage, edgeImage, 80, 200);

    // Hough Transform to detect lines
    auto raw_lines = std::vector<cv::Vec2f>();
    cv::HoughLines(edgeImage, raw_lines, 1, M_PI / 180.0, 50);

    // Filter to get the 4 card boundary lines
    auto card_lines = find_card_lines(raw_lines);

    auto corners = std::vector<cv::Point>();
    if (card_lines.size() == 4)
    {
      // Long lines are at index 0 and 1, short lines at 2 and 3
      const auto l_lines = std::vector<PolarLine>{card_lines[0], card_lines[1]};
      const auto s_lines = std::vector<PolarLine>{card_lines[2], card_lines[3]};

      for (const auto &ll : l_lines)
      {
        for (const auto &sl : s_lines)
        {
          cv::Point pt;
          if (get_intersection(ll, sl, pt))
          {
            corners.push_back(pt);
          }
        }
      }

      if (corners.size() == 4)
      {
        last_corners = sort_corners(corners);
      }
    }

    // Render detected corners and boundaries
    if (last_corners.size() == 4)
    {
      // Draw lines between corners (Red)
      for (size_t i = 0; i < 4; ++i)
      {
        cv::line(frameImage, last_corners[i], last_corners[(i + 1) % 4], cv::Scalar(0, 0, 255), 2, cv::LINE_AA);
      }
      // Draw circles on corners (Green)
      for (const auto &pt : last_corners)
      {
        cv::circle(frameImage, pt, 6, cv::Scalar(0, 255, 0), -1, cv::LINE_AA);
      }
    }

    writer.write(frameImage);

    if (frame_index % 100 == 0)
    {
      std::cout << "Processing frame " << frame_index << "... Corners detected: " << (corners.size() == 4 ? "Success" : "Failed (using fallback)") << std::endl;
    }
    frame_index++;
  }

  capture.release();
  writer.release();

  std::cout << "\nProcessed " << frame_index << " frames." << std::endl;
  std::cout << "Detected card video generated: " << OUTPUT_FILENAME << std::endl;
  return 0;
}

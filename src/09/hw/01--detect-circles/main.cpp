#include <algorithm>
#include <iostream>
#include <numeric>
#include <opencv2/opencv.hpp>
#include <ranges>
#include <vector>

#include "../../utils/opencv.hpp"

#define INPUT_FILENAME "./assets/balls.mov"
#define OUTPUT_FILENAME "./out/detected_balls.mov"

typedef cv::Vec3f Circle;

std::vector<Circle> detect_circles(const cv::Mat &frame)
{
  auto gray = cv::Mat();
  cv::cvtColor(frame, gray, cv::COLOR_BGR2GRAY);

  auto circles = std::vector<Circle>();
  cv::HoughCircles(gray, circles, cv::HOUGH_GRADIENT, 1, 15, 50, 20, 6, 16);

  return circles;
}

void draw_detections(cv::Mat &frame, const std::vector<Circle> &circles)
{
  const auto cRed = cv::Scalar(0, 0, 255);
  const auto cGreen = cv::Scalar(0, 255, 0);

  for (const auto &c : circles)
  {
    const auto center = cv::Point(static_cast<int>(c[0]), static_cast<int>(c[1]));
    const int radius = static_cast<int>(c[2]);

    cv::circle(frame, center, radius, cGreen, 2, cv::LINE_AA);
    cv::circle(frame, center, 3, cRed, -1, cv::LINE_AA);
  }

  const auto text = "Circles detected: " + std::to_string(circles.size());
  cv::putText(frame, text, cv::Point(20, 40), cv::FONT_HERSHEY_SIMPLEX, 1.0, cv::Scalar(0, 0, 255), 2, cv::LINE_AA);
}

void print_circles_stat(const std::vector<int> &circle_counts, int frame_index)
{
  const double sum = std::accumulate(circle_counts.begin(), circle_counts.end(), 0.0);
  const double mean = sum / circle_counts.size();

  const auto min_val = *std::ranges::min_element(circle_counts);
  const auto max_val = *std::ranges::max_element(circle_counts);

  auto sorted_counts = circle_counts;
  std::ranges::sort(sorted_counts);
  const double median =
      (sorted_counts.size() % 2 == 0)
          ? (sorted_counts[sorted_counts.size() / 2 - 1] + sorted_counts[sorted_counts.size() / 2]) / 2.0
          : sorted_counts[sorted_counts.size() / 2];

  std::cout << "\n--- Circle Detection Statistics ---" << std::endl;
  std::cout << "Processed " << frame_index << " frames." << std::endl;
  std::cout << "Minimum detected circles: " << min_val << std::endl;
  std::cout << "Maximum detected circles: " << max_val << std::endl;
  std::cout << "Average detected circles: " << mean << std::endl;
  std::cout << "Median detected circles: " << median << std::endl;
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

  auto writer = cv::VideoWriter(OUTPUT_FILENAME, cv::VideoWriter::fourcc('m', 'p', '4', 'v'), fps, imageSize);

  if (!writer.isOpened())
  {
    std::cerr << "Failed to open video writer: " << OUTPUT_FILENAME << std::endl;
    return -1;
  }

  auto originalImage = cv::Mat();
  auto frameImage = cv::Mat(imageSize, CV_8UC3);
  auto circle_counts = std::vector<int>();
  int frame_index = 0;

  while (true)
  {
    capture >> originalImage;
    if (originalImage.empty())
    {
      break;
    }

    cv::resize(originalImage, frameImage, imageSize);

    auto circles = detect_circles(frameImage);
    circle_counts.push_back(static_cast<int>(circles.size()));

    draw_detections(frameImage, circles);
    writer.write(frameImage);

    if (frame_index % 100 == 0)
    {
      std::cout << "Processing frame " << frame_index << "... Detected: " << circles.size() << std::endl;
    }
    frame_index++;
  }

  capture.release();
  writer.release();

  if (!circle_counts.empty())
  {
    print_circles_stat(circle_counts, frame_index);
  }

  std::cout << "\nOutput video generated: " << OUTPUT_FILENAME << std::endl;
  return 0;
}

#include <algorithm>
#include <cmath>
#include <iostream>
#include <numeric>
#include <opencv2/opencv.hpp>
#include <ranges>
#include <vector>

#include "../../utils/opencv.hpp"

#define INPUT_FILENAME "./assets/swingcar.mov"
#define OUTPUT_FILENAME "./out/stabilized_car.mov"

void draw_lines(cv::Mat &frame, const std::vector<cv::Vec2f> &lines)
{
  for (const auto &line : lines)
  {
    const float rho = line[0];
    const float theta = line[1];
    const double a = std::cos(theta);
    const double b = std::sin(theta);
    const double x0 = a * rho;
    const double y0 = b * rho;

    const auto p1 = cv::Point(cvRound(x0 - 1000 * b), cvRound(y0 + 1000 * a));
    const auto p2 = cv::Point(cvRound(x0 + 1000 * b), cvRound(y0 - 1000 * a));

    cv::line(frame, p1, p2, cv::Scalar(0, 0, 255), 2, cv::LINE_AA);
  }
}

cv::Mat stabilize_frame(const cv::Mat &frame, double angle)
{
  const auto center = cv::Point2f(frame.cols / 2.0f, frame.rows / 2.0f);
  const auto M = cv::getRotationMatrix2D(center, angle, 1.0);
  auto stabilized = cv::Mat();

  cv::warpAffine(frame, stabilized, M, frame.size(), cv::INTER_LINEAR, cv::BORDER_CONSTANT, cv::Scalar(255, 255, 255));
  return stabilized;
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

  const auto outputSize = cv::Size(w * 2, h);
  auto writer = cv::VideoWriter(OUTPUT_FILENAME, cv::VideoWriter::fourcc('m', 'p', '4', 'v'), fps, outputSize);

  if (!writer.isOpened())
  {
    std::cerr << "Failed to open video writer: " << OUTPUT_FILENAME << std::endl;
    return -1;
  }

  cv::namedWindow("Original", cv::WINDOW_AUTOSIZE);
  cv::namedWindow("Stabilized", cv::WINDOW_AUTOSIZE);
  cv::moveWindow("Original", 0, 0);
  cv::moveWindow("Stabilized", w + 20, 0);

  auto originalImage = cv::Mat();
  auto frameImage = cv::Mat(imageSize, CV_8UC3);
  auto grayImage = cv::Mat(imageSize, CV_8UC1);
  auto edgeImage = cv::Mat(imageSize, CV_8UC1);

  double current_angle = 0.0;
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

    // Generate edge image
    cv::Canny(grayImage, edgeImage, 50, 150, 3);

    // Detect lines using Hough Transform
    auto lines = std::vector<cv::Vec2f>();
    cv::HoughLines(edgeImage, lines, 1, M_PI / 180.0, 150);

    // Filter lines that are roughly horizontal (theta close to 90 degrees / PI/2)
    // We keep lines where |theta - PI/2| < 15 degrees
    const double angle_tolerance_rad = 15.0 * M_PI / 180.0;
    auto pred = [angle_tolerance_rad](const cv::Vec2f &line) {
      const float theta = line[1];
      return std::abs(theta - static_cast<float>(M_PI / 2.0)) < angle_tolerance_rad;
    };
    auto horizontal_lines = lines | std::views::filter(pred);

    // Convert view to vector of lines for drawing and processing
    auto detected_horiz_lines = std::vector<cv::Vec2f>();
    for (const auto &line : horizontal_lines)
    {
      detected_horiz_lines.push_back(line);
    }

    if (!detected_horiz_lines.empty())
    {
      // Calculate average theta
      double sum_theta = 0.0;
      for (const auto &line : detected_horiz_lines)
      {
        sum_theta += line[1];
      }
      const double avg_theta = sum_theta / detected_horiz_lines.size();

      // Convert theta to degrees and compute the tilt angle relative to horizontal (90 degrees)
      const double avg_theta_deg = avg_theta * 180.0 / M_PI;
      const double target_angle = avg_theta_deg - 90.0;

      // Smooth the angle using a simple low-pass filter (Exponential Moving Average) to avoid jitter
      current_angle = current_angle + 0.5 * (target_angle - current_angle);
    }
    // If no horizontal lines are detected, we retain the current_angle from the previous frame.

    // Create the left (original + detected lines) and right (stabilized) frames (no text overlays)
    auto leftFrame = frameImage.clone();
    draw_lines(leftFrame, detected_horiz_lines);

    auto rightFrame = stabilize_frame(frameImage, current_angle);

    // Show windows separately and simultaneously
    cv::imshow("Original", leftFrame);
    cv::imshow("Stabilized", rightFrame);

    // Merge left and right frames side-by-side for the output video
    auto outputFrame = cv::Mat(outputSize, CV_8UC3);
    auto leftRoi = outputFrame(cv::Rect(0, 0, w, h));
    auto rightRoi = outputFrame(cv::Rect(w, 0, w, h));
    leftFrame.copyTo(leftRoi);
    rightFrame.copyTo(rightRoi);

    writer.write(outputFrame);

    const int key = cv::waitKey(10);
    if (key == 'q')
    {
      break;
    }

    if (frame_index % 100 == 0)
    {
      std::cout << "Processing frame " << frame_index << "... Tilt: " << current_angle << " deg" << std::endl;
    }
    frame_index++;
  }

  capture.release();
  writer.release();
  cv::destroyAllWindows();

  std::cout << "\nProcessed " << frame_index << " frames." << std::endl;
  std::cout << "Stabilized video generated: " << OUTPUT_FILENAME << std::endl;
  return 0;
}

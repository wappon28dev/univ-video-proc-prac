#include <algorithm>
#include <array>
#include <cmath>
#include <iostream>
#include <ranges>
#include <vector>

#include <opencv2/opencv.hpp>

#include "../../utils/opencv.hpp"

constexpr auto INPUT_FILENAME = "./assets/obj.mov";
constexpr auto OUTPUT_FILENAME = "./out/output.mp4";

cv::VideoCapture open_capture(const char *filename)
{
  auto capture = cv::VideoCapture(filename);
  if (!capture.isOpened())
  {
    std::cerr << "Error: Could not open video file." << std::endl;
  }

  return capture;
}

cv::VideoWriter create_writer(const cv::VideoCapture &capture)
{
  auto frame_width = static_cast<int>(capture.get(cv::CAP_PROP_FRAME_WIDTH));
  auto frame_height = static_cast<int>(capture.get(cv::CAP_PROP_FRAME_HEIGHT));

  auto frame_size = cv::Size(frame_width * 2, frame_height);
  auto frame_fps = capture.get(cv::CAP_PROP_FPS);
  if (frame_fps <= 0.0)
  {
    frame_fps = 30.0;
  }
  auto fourcc_code = cv::VideoWriter::fourcc('m', 'p', '4', 'v');

  return cv::VideoWriter(OUTPUT_FILENAME, fourcc_code, frame_fps, frame_size);
}

using Contour = std::vector<cv::Point>;

int main(int argc, char *argv[], char *envp[])
{
  auto capture = open_capture(INPUT_FILENAME);
  if (!capture.isOpened())
  {
    return -1;
  }

  auto output_writer = create_writer(capture);
  if (!output_writer.isOpened())
  {
    std::cerr << "Error: Could not open VideoWriter." << std::endl;
    return -1;
  }

  auto frame_idx = 0;
  while (true)
  {
    auto frame = cv::Mat();
    capture >> frame;
    if (frame.empty())
    {
      break;
    }

    auto hsv = cv::Mat();
    cv::cvtColor(frame, hsv, cv::COLOR_BGR2HSV);

    auto mask_yellow = cv::Mat();
    auto mask_blue = cv::Mat();

    // Yellow (Stars): Hue ~ 15-35, Saturation ~ 150-255, Value ~ 150-255
    cv::inRange(hsv, cv::Scalar(15, 150, 150), cv::Scalar(35, 255, 255), mask_yellow);
    // Blue (Pentagons): Hue ~ 100-130, Saturation ~ 150-255, Value ~ 150-255
    cv::inRange(hsv, cv::Scalar(100, 150, 150), cv::Scalar(130, 255, 255), mask_blue);

    // Find contours
    auto contours_yellow = std::vector<Contour>();
    auto contours_blue = std::vector<Contour>();
    cv::findContours(mask_yellow.clone(), contours_yellow, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);
    cv::findContours(mask_blue.clone(), contours_blue, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);

    // Save check frames (every 50 frames)
    if (frame_idx % 50 == 0)
    {
      cv::imwrite("./out/check_frame_" + std::to_string(frame_idx) + ".png", frame);
    }

    // Prepare black canvas for drawing contours
    cv::Mat contour_frame = cv::Mat::zeros(frame.size(), CV_8UC3);

    // Process yellow contours (Stars)
    for (const auto &contour : contours_yellow)
    {
      auto area = cv::contourArea(contour);
      if (area < 300 || area > 5000)
      {
        continue;
      }

      auto hull = Contour();
      cv::convexHull(contour, hull);
      auto hull_area = cv::contourArea(hull);
      auto solidness = (hull_area > 0) ? (area / hull_area) : 0.0;

      // Relaxed Star criteria: Solidness (0.4 to 0.85)
      if (solidness >= 0.4 && solidness <= 0.85)
      {
        // Draw contour in yellow (BGR: 0, 255, 255)
        cv::drawContours(contour_frame, std::vector<Contour>{contour}, -1, cv::Scalar(0, 255, 255), 2);
      }
    }

    // Process blue contours (Pentagons)
    for (const auto &contour : contours_blue)
    {
      auto area = cv::contourArea(contour);
      if (area < 300 || area > 5000)
      {
        continue;
      }

      auto hull = Contour();
      cv::convexHull(contour, hull);
      auto hull_area = cv::contourArea(hull);
      auto solidness = (hull_area > 0) ? (area / hull_area) : 0.0;

      auto approx = Contour();
      auto peri = cv::arcLength(contour, true);
      cv::approxPolyDP(contour, approx, 0.02 * peri, true);
      auto vertices = approx.size();

      // Pentagon criteria: Solidness (>= 0.85)
      if (solidness >= 0.85)
      {
        // Draw contour in blue (BGR: 255, 0, 0)
        cv::drawContours(contour_frame, std::vector<Contour>{contour}, -1, cv::Scalar(255, 0, 0), 2);
      }
    }

    auto combined = cv::Mat();
    cv::hconcat(frame, contour_frame, combined);

    output_writer << combined;

    frame_idx++;
  }

  capture.release();
  output_writer.release();

  std::cout << "Processing completed. Output saved to: " << OUTPUT_FILENAME << std::endl;
  return 0;
}

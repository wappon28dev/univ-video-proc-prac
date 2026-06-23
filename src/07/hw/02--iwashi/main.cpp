#include <algorithm>
#include <array>
#include <cmath>
#include <iostream>
#include <ranges>
#include <vector>

#include <opencv2/opencv.hpp>

#include "../../utils/opencv.hpp"

constexpr auto INPUT_FILENAME = "./assets/iwashi.mov";
constexpr auto OUTPUT_FILENAME = "./out/iwashi-flow.mp4";
constexpr auto MOTION_LEN_THRESHOLD_MAX = 20;

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
  auto frame_size = cv::Size(frame_width, frame_height);
  auto frame_fps = 30.0; // capture.get(cv::CAP_PROP_FPS);
  auto fourcc_code = cv::VideoWriter::fourcc('m', 'p', '4', 'v');

  return cv::VideoWriter(OUTPUT_FILENAME, fourcc_code, frame_fps, frame_size);
}

cv::Mat to_gray(const cv::Mat &frame)
{
  auto gray = cv::Mat();
  cv::cvtColor(frame, gray, cv::COLOR_BGR2GRAY);
  return gray;
}

typedef std::vector<cv::Point2f> FeaturePoints;

FeaturePoints detect_features(const cv::Mat &gray)
{
  auto features = FeaturePoints();
  cv::goodFeaturesToTrack(gray, features, 300, 0.01, 10);
  return features;
}

auto is_active_points(const auto &status, const auto &points_next)
{
  return [&](const cv::Point2f &pt) -> bool {
    auto idx = &pt - points_next.data();
    return idx < status.size() && status[idx] != 0;
  };
}

cv::Scalar direction_color(const cv::Point2f &delta)
{
  static const std::array<cv::Scalar, 8> colors = {cv::Scalar(0, 0, 255),   //
                                                   cv::Scalar(0, 128, 255), //
                                                   cv::Scalar(0, 255, 255), //
                                                   cv::Scalar(0, 255, 0),   //
                                                   cv::Scalar(255, 255, 0), //
                                                   cv::Scalar(255, 0, 0),   //
                                                   cv::Scalar(255, 0, 128), //
                                                   cv::Scalar(255, 0, 255)};

  auto angle = std::atan2(static_cast<double>(delta.y), static_cast<double>(delta.x));
  if (angle < 0.0)
  {
    angle += 2.0 * CV_PI;
  }

  auto bin = static_cast<int>(angle / (2.0 * CV_PI / 8.0)) % 8;
  return colors[bin];
}

cv::Mat build_motion_canvas(const cv::Size &size, const FeaturePoints &points_prev, const FeaturePoints &points_next,
                            const std::vector<unsigned char> &status)
{
  cv::Mat motion_canvas = cv::Mat::zeros(size, CV_8UC3);

  for (auto const &[prev, next, st] : std::views::zip(points_prev, points_next, status))
  {
    if (st == 0)
    {
      continue;
    }

    auto pt1 = cv::Point(prev);
    auto pt2 = cv::Point(next);
    auto delta = next - prev;
    auto length = std::hypot(delta.x, delta.y);
    auto color = direction_color(delta);

    if (length > MOTION_LEN_THRESHOLD_MAX)
    {
      continue;
    }

    cv::line(motion_canvas, pt1, pt2, color, 2);
  }

  return motion_canvas;
}

cv::Mat build_motion_mask(const cv::Mat &motion_canvas)
{
  auto motion_mask = cv::Mat();
  cv::cvtColor(motion_canvas, motion_mask, cv::COLOR_BGR2GRAY);
  cv::threshold(motion_mask, motion_mask, 1, 255, cv::THRESH_BINARY);
  return motion_mask;
}

void accumulate_motion_mask(cv::Mat &motion_mask_prev, const cv::Mat &motion_mask)
{
  if (motion_mask_prev.empty())
  {
    motion_mask.copyTo(motion_mask_prev);
    return;
  }

  cv::bitwise_or(motion_mask_prev, motion_mask, motion_mask_prev);
}

void accumulate_motion_frame(cv::Mat &frame_out, const cv::Mat &motion_canvas)
{
  if (frame_out.empty())
  {
    frame_out = cv::Mat::zeros(motion_canvas.size(), motion_canvas.type());
  }

  frame_out *= 0.99;

  cv::add(frame_out, motion_canvas, frame_out);
}

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

  auto previous_gray = cv::Mat();
  auto previous_points = FeaturePoints();
  auto motion_mask_prev = cv::Mat();
  auto frame_out = cv::Mat();

  auto frame_idx = 0;
  while (true)
  {
    auto frame = cv::Mat();
    capture >> frame;
    if (frame.empty())
    {
      break;
    }

    auto gray = to_gray(frame);

    if (previous_gray.empty() || frame_idx % 5 == 0)
    {
      previous_gray = gray;
      previous_points = detect_features(gray);
    }

    auto points_next = FeaturePoints();
    auto status = std::vector<unsigned char>();
    auto errors = std::vector<float>();

    if (!previous_points.empty())
    {
      cv::TermCriteria criteria = cv::TermCriteria(cv::TermCriteria::MAX_ITER | cv::TermCriteria::EPS, 20, 0.05);
      cv::calcOpticalFlowPyrLK(previous_gray, gray, previous_points, points_next, status, errors, cv::Size(10, 10), 4,
                               criteria);
    }

    auto motion_canvas = build_motion_canvas(frame.size(), previous_points, points_next, status);
    auto motion_mask = build_motion_mask(motion_canvas);
    accumulate_motion_mask(motion_mask_prev, motion_mask);
    accumulate_motion_frame(frame_out, motion_canvas);

    previous_gray = gray;
    previous_points =
        points_next | std::views::filter(is_active_points(status, points_next)) | std::ranges::to<FeaturePoints>();

    mat_util::show("Frame", frame);
    mat_util::show("OpticalFlow", frame_out);
    output_writer << frame_out;

    auto key = cv::waitKey(20);
    if (key == 'q')
    {
      break;
    }

    frame_idx++;
  }

  capture.release();

  return 0;
}

#include <algorithm>
#include <array>
#include <cmath>
#include <iostream>
#include <ranges>
#include <vector>

#include <opencv2/opencv.hpp>

#include "../../utils/opencv.hpp"

constexpr auto INPUT_FILENAME = "./assets/room.mov";
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
  auto frame_size = cv::Size(frame_width, frame_height);
  auto frame_fps = capture.get(cv::CAP_PROP_FPS);
  if (frame_fps <= 0.0)
  {
    frame_fps = 30.0;
  }
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
  cv::goodFeaturesToTrack(gray, features, 200, 0.01, 10);
  return features;
}

auto is_active_points(const auto &status, const auto &points_next)
{
  return [&](const cv::Point2f &pt) -> bool {
    auto idx = &pt - points_next.data();
    return idx < status.size() && status[idx] != 0;
  };
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

    auto points_next = FeaturePoints();
    auto status = std::vector<unsigned char>();
    auto errors = std::vector<float>();

    bool has_flow = false;
    if (!previous_gray.empty() && !previous_points.empty())
    {
      cv::TermCriteria criteria = cv::TermCriteria(cv::TermCriteria::MAX_ITER | cv::TermCriteria::EPS, 20, 0.05);
      cv::calcOpticalFlowPyrLK(previous_gray, gray, previous_points, points_next, status, errors, cv::Size(15, 15), 4,
                               criteria);
      has_flow = true;
    }

    double sum_dx = 0.0;
    double sum_dy = 0.0;
    int count = 0;

    if (has_flow)
    {
      for (size_t i = 0; i < previous_points.size(); ++i)
      {
        if (status[i] != 0)
        {
          auto delta = points_next[i] - previous_points[i];
          double length = std::hypot(delta.x, delta.y);

          if (length < 50.0)
          {
            sum_dx += delta.x;
            sum_dy += delta.y;
            count++;
          }
        }
      }
    }

    cv::Point2f camera_motion(0.0f, 0.0f);
    if (count > 0)
    {
      camera_motion.x = -static_cast<float>(sum_dx / count);
      camera_motion.y = -static_cast<float>(sum_dy / count);
    }

    auto output_frame = frame.clone();

    cv::Point center(frame.cols / 2, frame.rows / 2);

    float scale = 10.0f;
    cv::Point arrow_end =
        center + cv::Point(static_cast<int>(camera_motion.x * scale), static_cast<int>(camera_motion.y * scale));

    double motion_len = std::hypot(camera_motion.x, camera_motion.y);
    if (motion_len > 0.3)
    {
      cv::arrowedLine(output_frame, center, arrow_end, cv::Scalar(0, 0, 255), 4, cv::LINE_AA, 0, 0.25);
    }

    if (frame_idx == 51 || frame_idx == 151 || frame_idx == 301)
    {
      cv::imwrite("./out/check_frame_" + std::to_string(frame_idx) + ".png", output_frame);
    }

    previous_gray = gray;
    if (frame_idx % 10 == 0 || previous_points.empty())
    {

      previous_points = detect_features(gray);
    }
    else
    {
      previous_points =
          points_next | std::views::filter(is_active_points(status, points_next)) | std::ranges::to<FeaturePoints>();
    }

    output_writer << output_frame;

    mat_util::show("Original", frame);
    mat_util::show("Camera Motion", output_frame);

    auto key = cv::waitKey(20);
    if (key == 'q')
    {
      break;
    }

    frame_idx++;
  }

  capture.release();
  output_writer.release();

  std::cout << "Camera motion tracking completed. Output saved to: " << OUTPUT_FILENAME << std::endl;
  return 0;
}

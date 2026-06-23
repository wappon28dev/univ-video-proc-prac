#include <iostream>
#include <opencv2/opencv.hpp>

#include "../../utils/opencv.hpp"

#define INPUT_FILENAME "./assets/iwashi.mov"
#define OUTPUT_FILENAME "./out/iwashi.mp4"

cv::VideoWriter get_writer(const cv::VideoCapture &capture)
{
  auto frame_width = static_cast<int>(capture.get(cv::CAP_PROP_FRAME_WIDTH));
  auto frame_height = static_cast<int>(capture.get(cv::CAP_PROP_FRAME_HEIGHT));
  auto frame_size = cv::Size(frame_width, frame_height);
  auto frame_fps = capture.get(cv::CAP_PROP_FPS);

  auto fourcc_code = cv::VideoWriter::fourcc('m', 'p', '4', 'v');
  return cv::VideoWriter(OUTPUT_FILENAME, fourcc_code, frame_fps, frame_size);
}

cv::Mat draw_circle(const cv::Mat &frame, const cv::Mat &frame_sub)
{
  auto frame_out = frame.clone();
  auto area = cv::countNonZero(frame_sub);
  if (area <= 0)
  {
    return frame_out;
  }

  auto radius = static_cast<int>(std::sqrt(area / CV_PI));
  radius = std::max(1, radius);
  auto center = cv::Point(frame.cols / 2, frame.rows / 2);
  cv::circle(frame_out, center, radius, cv::Scalar(0, 0, 255), 2);

  return frame_out;
}

int main(int argc, char *argv[], char *envp[])
{

  auto capture = cv::VideoCapture(INPUT_FILENAME);

  if (!capture.isOpened())
  {
    std::cerr << "Error: Could not open video file." << std::endl;
    return -1;
  }

  auto output_writer = get_writer(capture);
  if (!output_writer.isOpened())
  {
    std::cerr << "Error: Could not open VideoWriter." << std::endl;
    return -1;
  }

  auto frame = cv::Mat();
  auto frame_prev = cv::Mat();

  while (true)
  {
    capture >> frame;
    if (frame.empty())
    {
      break;
    }

    if (frame_prev.empty())
    {
      frame_prev = frame.clone();
      continue;
    }

    auto frame_sub = cv::Mat();
    cv::absdiff(frame, frame_prev, frame_sub);

    // マスク準備
    cv::cvtColor(frame_sub, frame_sub, cv::COLOR_BGR2GRAY);
    cv::threshold(frame_sub, frame_sub, 30, 255, cv::THRESH_BINARY);

    auto frame_out = cv::Mat();
    frame.copyTo(frame_out, frame_sub);

    // 円の描画
    frame_out = draw_circle(frame_out, frame_sub);

    mat_util::show("Input", frame);
    mat_util::show("Output", frame_out);

    output_writer << frame_out;
    frame_prev = frame.clone();

    auto key = cv::waitKey(20);
    if (key == 'q')
    {
      break;
    }
  }

  capture.release();

  return 0;
}

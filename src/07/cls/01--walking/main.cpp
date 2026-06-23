#include <iostream>
#include <opencv2/opencv.hpp>

#include "../../utils/opencv.hpp"

#define INPUT_FILENAME "./assets/walking.mov"
#define OUTPUT_FILENAME "./out/walking.mp4"

cv::VideoWriter get_writer(const cv::VideoCapture &capture)
{
  auto frame_width = static_cast<int>(capture.get(cv::CAP_PROP_FRAME_WIDTH));
  auto frame_height = static_cast<int>(capture.get(cv::CAP_PROP_FRAME_HEIGHT));
  auto frame_size = cv::Size(frame_width, frame_height);
  auto frame_fps = capture.get(cv::CAP_PROP_FPS);

  auto fourcc_code = cv::VideoWriter::fourcc('m', 'p', '4', 'v');
  return cv::VideoWriter(OUTPUT_FILENAME, fourcc_code, frame_fps, frame_size);
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

    mat_util::show("Input", frame);
    mat_util::show("Output", frame_sub);

    output_writer << frame_sub;
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

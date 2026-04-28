#include <iostream>
#include <opencv2/opencv.hpp>

#include "../../utils/opencv.hpp"

#define OUTPUT_FILENAME "out/reg.mp4"

int main(int argc, char *argv[], char *envp[])
{
  auto capture = cv::VideoCapture(0);

  if (!capture.isOpened())
  {
    std::cerr << "Error: Could not open camera." << std::endl;
    return -1;
  }

  auto frame_width = static_cast<int>(capture.get(cv::CAP_PROP_FRAME_WIDTH));
  auto frame_height = static_cast<int>(capture.get(cv::CAP_PROP_FRAME_HEIGHT));
  auto frame_size = cv::Size(frame_width, frame_height);
  auto frame_fps = capture.get(cv::CAP_PROP_FPS);

  auto fourcc_code = cv::VideoWriter::fourcc('m', 'p', '4', 'v');
  auto output_writer = cv::VideoWriter(OUTPUT_FILENAME, fourcc_code, frame_fps, frame_size);

  if (!output_writer.isOpened())
  {
    std::cerr << "Error: Could not open VideoWriter." << std::endl;
    return -1;
  }

  auto frame = cv::Mat();
  auto frame_gray = cv::Mat();
  auto frame_binary = cv::Mat();
  auto frame_out = cv::Mat();

  while (true)
  {
    capture >> frame;
    if (frame.empty())
    {
      std::cerr << "Error: Empty frame captured." << std::endl;
      break;
    }

    cv::cvtColor(frame, frame_gray, cv::COLOR_BGR2GRAY);
    cv::threshold(frame_gray, frame_binary, 100, 255, cv::THRESH_BINARY);
    cv::cvtColor(frame_binary, frame_out, cv::COLOR_GRAY2BGR);
    output_writer << frame_out;

    mat_util::show("Camera", frame);
    mat_util::show("Binary", frame_binary);

    auto key = cv::waitKey(20);
    if (key == 'q')
    {
      break;
    }
  }

  cv::destroyAllWindows();
  return 0;
}

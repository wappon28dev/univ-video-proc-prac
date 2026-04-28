#include <iostream>
#include <opencv2/opencv.hpp>

#include "../../utils/opencv.hpp"

#define INPUT_FILENAME_FOREGROUND "assets/dance.mov"
#define INPUT_FILENAME_BACKGROUND "assets/264490_tiny.mp4"
#define OUTPUT_FILENAME "out/dance.mp4"

int main(int argc, char *argv[], char *envp[])
{
  cv::VideoCapture input_foreground(INPUT_FILENAME_FOREGROUND);
  if (!input_foreground.isOpened())
  {
    std::cerr << "Error: Could not open video file: " << INPUT_FILENAME_FOREGROUND << std::endl;
    return -1;
  }

  cv::VideoCapture input_background(INPUT_FILENAME_BACKGROUND);
  if (!input_background.isOpened())
  {
    std::cerr << "Error: Could not open video file: " << INPUT_FILENAME_BACKGROUND << std::endl;
    return -1;
  }

  auto frame_width = static_cast<int>(input_foreground.get(cv::CAP_PROP_FRAME_WIDTH));
  auto frame_height = static_cast<int>(input_foreground.get(cv::CAP_PROP_FRAME_HEIGHT));
  auto frame_size = cv::Size(frame_width, frame_height);
  auto frame_fps = input_foreground.get(cv::CAP_PROP_FPS);

  auto fourcc_code = cv::VideoWriter::fourcc('m', 'p', '4', 'v');
  cv::VideoWriter output_writer(OUTPUT_FILENAME, fourcc_code, frame_fps, frame_size);

  if (!output_writer.isOpened())
  {
    std::cerr << "Error: Could not open video writer: " << OUTPUT_FILENAME << std::endl;
    return -1;
  }

  cv::Mat frame_fg, frame_bg, frame_out;
  while (true)
  {
    input_foreground >> frame_fg;
    input_background >> frame_bg;

    if (frame_fg.empty() || frame_bg.empty())
    {
      std::cout << "End of video stream reached." << std::endl;
      break;
    }

    cv::resize(frame_bg, frame_bg, frame_size);

    cv::Mat hsv, mask;
    cv::cvtColor(frame_fg, hsv, cv::COLOR_BGR2HSV);
    cv::inRange(hsv, cv::Scalar(40, 160, 150), cv::Scalar(50, 255, 255), mask);

    cv::Mat mask_inv;
    cv::bitwise_not(mask, mask_inv);

    frame_out = frame_bg.clone();
    frame_fg.copyTo(frame_out, mask_inv);

    cv::imshow("Composited Video", frame_out);
    output_writer << frame_out;

    auto key = cv::waitKey(30);
    if (key == 'q')
    {
      std::cout << "Exiting video playback." << std::endl;
      break;
    }
  }

  cv::destroyAllWindows();
  return 0;
}

#include <iostream>
#include <opencv2/opencv.hpp>

#include "../../utils/opencv.hpp"

#define INPUT_FILENAME "assets/dance.mov"

int main(int argc, char *argv[], char *envp[])
{

  auto capture = cv::VideoCapture(INPUT_FILENAME);

  if (!capture.isOpened())
  {
    std::cerr << "Error: Could not open video file: " << INPUT_FILENAME << std::endl;
    return -1;
  }

  auto w = 640;
  auto h = 360;
  auto out = cv::Mat(cv::Size(w, h), CV_8UC3);
  auto frame = cv::Mat();

  const auto win_frame_name = "Frame";
  cv::namedWindow(win_frame_name);
  cv::moveWindow(win_frame_name, 0, 0);

  // const auto win_result_name = "Result";
  // cv::namedWindow(win_result_name);
  // cv::moveWindow(win_result_name, 0, h);

  while (true)
  {
    capture >> frame;
    if (frame.empty())
    {
      std::cerr << "Error: Empty frame captured." << std::endl;
      break;
    }

    cv::imshow(win_frame_name, frame);
    // cv::imshow(win_result_name, out);

    auto key = cv::waitKey(20);
    if (key == 'q')
    {
      break;
    }
  }

  return 0;
}

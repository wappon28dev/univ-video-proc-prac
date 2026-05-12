#include <iostream>
#include <opencv2/opencv.hpp>

#include "../../utils/opencv.hpp"

#define OUTPUT_FILENAME "out/reg.mp4"

std::vector<int> get_lut(int level = 4)
{
  auto lut_size = 256;
  auto color_max = 255;

  if (level <= 1)
  {
    return std::vector<int>(lut_size, 0);
  }

  std::vector<int> lut(lut_size);
  auto step = static_cast<double>(lut_size) / level;

  for (int i = 0; i < lut_size; ++i)
  {
    auto idx = std::min(static_cast<int>(i / step), level - 1);
    auto is_last = (idx == level - 1);

    lut[i] = is_last ? 255 : static_cast<int>(step * idx + step / 2);
  }
  return lut;
}

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

  while (true)
  {
    capture >> frame;
    if (frame.empty())
    {
      std::cerr << "Error: Empty frame captured." << std::endl;
      break;
    }

    auto frame_gray = cv::Mat();
    cv::cvtColor(frame, frame_gray, cv::COLOR_BGR2GRAY);
    auto lut = get_lut(6);

    auto frame_posterized = frame_gray.clone();
    frame_posterized.forEach<uchar>([&lut](uchar &pixel, const int *position) -> void { pixel = lut[pixel]; });

    auto frame_out = cv::Mat();
    cv::cvtColor(frame_posterized, frame_out, cv::COLOR_GRAY2BGR);
    output_writer << frame_out;

    mat_util::show("Camera", frame);
    mat_util::show("Posterized", frame_posterized);

    auto key = cv::waitKey(20);
    if (key == 'q')
    {
      break;
    }
  }

  cv::destroyAllWindows();
  return 0;
}

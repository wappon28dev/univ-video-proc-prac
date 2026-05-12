#include <iostream>
#include <opencv2/opencv.hpp>

#include "../../utils/opencv.hpp"

#define OUTPUT_FILENAME "out/reg.mp4"
#define BLOCK_SIZE 4
#define LEVELS 17

// ref: https://ja.wikipedia.org/wiki/%E9%85%8D%E5%88%97%E3%83%87%E3%82%A3%E3%82%B6%E3%83%AA%E3%83%B3%E3%82%B0
const auto DITHERING_MAT = std::array<std::array<int, BLOCK_SIZE>, BLOCK_SIZE>{{
    {0, 8, 2, 10},
    {12, 4, 14, 6},
    {3, 11, 1, 9},
    {15, 7, 13, 5},
}};

int to_level(const uchar pixel)
{
  return std::clamp((pixel * (LEVELS - 1) + 127) / 255, 0, LEVELS - 1);
}

uchar level_to_pixel(const int level)
{
  return static_cast<uchar>(std::clamp((level * 255 + (LEVELS - 2) / 2) / (LEVELS - 1), 0, 255));
}

cv::Mat make_dither_block(const int level)
{
  auto block = cv::Mat(BLOCK_SIZE, BLOCK_SIZE, CV_8UC1, cv::Scalar(0));
  block.forEach<uchar>([&](uchar &pixel, const int *position) -> void {
    const auto row = position[0];
    const auto col = position[1];
    pixel = DITHERING_MAT[row][col] < level ? 255 : 0;
  });
  return block;
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
  auto frame_fps = 10;

  auto fourcc_code = cv::VideoWriter::fourcc('m', 'p', '4', 'v');
  auto output_writer = cv::VideoWriter("out/reg.mp4", fourcc_code, frame_fps, frame_size);

  if (!output_writer.isOpened())
  {
    std::cerr << "Error: Could not open VideoWriter." << std::endl;
    return -1;
  }

  auto frame = cv::Mat();
  auto frame_small = cv::Mat();
  auto frame_gray_small = cv::Mat();
  auto frame_half_tone = cv::Mat(frame_size, CV_8UC1, cv::Scalar(0));
  auto frame_out = cv::Mat();

  while (true)
  {
    capture >> frame;
    if (frame.empty())
    {
      std::cerr << "Error: Empty frame captured." << std::endl;
      break;
    }

    cv::resize(frame, frame_small, cv::Size(frame_width / BLOCK_SIZE, frame_height / BLOCK_SIZE));
    cv::cvtColor(frame_small, frame_gray_small, cv::COLOR_BGR2GRAY);

    frame_half_tone.setTo(0);

    frame_gray_small.forEach<uchar>([&](uchar &pixel, const int *position) -> void {
      auto stat = mat_util::stat(position, pixel);

      const auto level = to_level(pixel);
      const auto white_count = std::clamp(level, 0, LEVELS - 1);
      const auto block = make_dither_block(white_count);

      auto [x, y] = std::make_pair(stat.x * BLOCK_SIZE, stat.y * BLOCK_SIZE);
      block.copyTo(frame_half_tone(cv::Rect(x, y, BLOCK_SIZE, BLOCK_SIZE)));
    });

    cv::cvtColor(frame_half_tone, frame_out, cv::COLOR_GRAY2BGR);
    output_writer << frame_out;

    mat_util::show("Camera", frame);
    mat_util::show("HalfTone", frame_out);

    auto key = cv::waitKey(20);
    if (key == 'q')
    {
      break;
    }
  }

  cv::destroyAllWindows();
  return 0;
}

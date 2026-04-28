#include <iostream>
#include <opencv2/opencv.hpp>

#include "../../utils/opencv.hpp"

#define INPUT_FILENAME "assets/dance.mov"
#define OUTPUT_FILENAME "out/dance.mp4"

cv::Mat extract_dancer(const cv::Mat &frame)
{
  auto frame_hsv = cv::Mat();
  cv::cvtColor(frame, frame_hsv, cv::COLOR_BGR2HSV);

  auto is_green = [](const vec3b_util::Color &color) -> bool {
    auto [h, s, v] = color;
    return (h >= 40 && h <= 50) && (s >= 160) && (v >= 150);
  };

  frame_hsv.forEach<cv::Vec3b>([&](cv::Vec3b &pixel, const int *position) -> void {
    auto stat = mat_util::stat(position, pixel);

    pixel = is_green(stat.color) ? vec3b_util::from(cv::Scalar(0, 0, 0)) : pixel;
  });

  auto frame_rgb = frame_hsv.clone();
  cv::cvtColor(frame_hsv, frame_rgb, cv::COLOR_HSV2BGR);

  return frame_rgb;
}

int main(int argc, char *argv[], char *envp[])
{
  auto input = cv::VideoCapture(INPUT_FILENAME);

  if (!input.isOpened())
  {
    std::cerr << "Error: Could not open video file: " << INPUT_FILENAME << std::endl;
    return -1;
  }

  auto frame_width = static_cast<int>(input.get(cv::CAP_PROP_FRAME_WIDTH));
  auto frame_height = static_cast<int>(input.get(cv::CAP_PROP_FRAME_HEIGHT));
  auto frame_size = cv::Size(frame_width, frame_height);
  auto frame_fps = input.get(cv::CAP_PROP_FPS);

  auto fourcc_code = cv::VideoWriter::fourcc('m', 'p', '4', 'v');
  auto output_writer = cv::VideoWriter(OUTPUT_FILENAME, fourcc_code, frame_fps, frame_size);

  if (!output_writer.isOpened())
  {
    std::cerr << "Error: Could not open video writer: " << OUTPUT_FILENAME << std::endl;
    return -1;
  }

  auto frame = cv::Mat();
  auto frame_out = cv::Mat();
  while (true)
  {
    input >> frame;
    if (frame.empty())
    {
      std::cout << "End of video stream reached." << std::endl;
      break;
    }

    auto output_frame = extract_dancer(frame);

    mat_util::show("Dancer", frame);
    mat_util::show("Extracted Dancer", output_frame);

    output_writer << output_frame;

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

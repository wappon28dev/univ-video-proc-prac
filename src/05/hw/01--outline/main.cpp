#include <iostream>
#include <opencv2/opencv.hpp>

#include "../../utils/opencv.hpp"

#define INPUT_FILENAME "assets/scene.mov"
#define OUTPUT_FILENAME "out/reg.mp4"

float fdata_laplacian[] = {
    0, 1,  0, //
    1, -4, 1, //
    0, 1,  0  //
};

cv::Mat process(const cv::Mat &frame)
{
  auto frame_out = cv::Mat();

  const auto ksize = 5;
  const auto size = cv::Size(ksize, ksize);

  // ガウシアン
  cv::GaussianBlur(frame, frame_out, size, 0);
  cv::cvtColor(frame_out, frame_out, cv::COLOR_BGR2GRAY);

  // メディアン
  cv::medianBlur(frame, frame_out, ksize);
  cv::cvtColor(frame_out, frame_out, cv::COLOR_BGR2GRAY);

  // ラプラシアン
  {
    auto kernel = cv::Mat(size, CV_32F, fdata_laplacian);
    cv::filter2D(frame_out, frame_out, CV_32F, kernel);
    cv::convertScaleAbs(frame_out, frame_out);
  }

  // 2 値化
  cv::threshold(frame_out, frame_out, 100, 255, cv::THRESH_BINARY);

  cv::cvtColor(frame_out, frame_out, cv::COLOR_GRAY2BGR);
  return frame_out;
}

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
    std::cerr << "Error: Could not open camera." << std::endl;
    return -1;
  }

  auto output_writer = get_writer(capture);
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

    auto frame_out = process(frame);

    output_writer << frame_out;

    mat_util::show("Input", frame);
    mat_util::show("Output", frame_out);

    auto key = cv::waitKey(20);
    if (key == 'q')
    {
      break;
    }
  }

  cv::destroyAllWindows();
  return 0;
}

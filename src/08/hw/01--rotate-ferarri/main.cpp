#include <algorithm>
#include <iostream>
#include <opencv2/opencv.hpp>

#include "../../utils/opencv.hpp"

#define BACKGROUND_FILENAME "./assets/milano.jpg"
#define FOREGROUND_FILENAME "./assets/ferarri.jpg"
#define OUTPUT_FILENAME "./out/output.mp4"

int main(int argc, char *argv[], char *envp[])
{

  auto bg = cv::imread(BACKGROUND_FILENAME);
  if (bg.empty())
  {
    std::cerr << "Failed to load background image: " << BACKGROUND_FILENAME << std::endl;
    return -1;
  }

  auto fg = cv::imread(FOREGROUND_FILENAME);
  if (fg.empty())
  {
    std::cerr << "Failed to load foreground image: " << FOREGROUND_FILENAME << std::endl;
    return -1;
  }

  int width = bg.cols;
  int height = bg.rows;
  cv::Size frame_size(width, height);

  double fps = 30.0;
  int total_frames = 300;
  auto fourcc = cv::VideoWriter::fourcc('m', 'p', '4', 'v');
  cv::VideoWriter writer(OUTPUT_FILENAME, fourcc, fps, frame_size);
  if (!writer.isOpened())
  {
    std::cerr << "Failed to open VideoWriter: " << OUTPUT_FILENAME << std::endl;
    return -1;
  }

  auto mask = cv::Mat(fg.size(), CV_8UC1, cv::Scalar(255));

  cv::Point2f center(fg.cols / 2.0f, fg.rows / 2.0f);

  int frame_count = 0;
  std::cout << "Starting video generation loop..." << std::endl;

  while (true)
  {
    if (frame_count >= total_frames)
    {
      break;
    }

    auto progress = static_cast<double>(frame_count) / total_frames;
    auto scale = 1.0 - progress;
    auto angle = progress * 1080.0;

    auto rotateMat = cv::getRotationMatrix2D(center, angle, scale);

    auto warped_fg = cv::Mat();
    cv::warpAffine(fg, warped_fg, rotateMat, frame_size, cv::INTER_LINEAR, cv::BORDER_CONSTANT, cv::Scalar(0, 0, 0));

    auto warped_mask = cv::Mat();
    cv::warpAffine(mask, warped_mask, rotateMat, frame_size, cv::INTER_NEAREST, cv::BORDER_CONSTANT, cv::Scalar(0));

    auto frame = bg.clone();
    warped_fg.copyTo(frame, warped_mask);

    mat_util::show("Composition Preview", frame);

    writer.write(frame);

    int key = cv::waitKey(33);
    if (key == 'q')
    {
      std::cout << "User interrupted execution." << std::endl;
      break;
    }

    frame_count++;
  }

  writer.release();
  cv::destroyAllWindows();

  std::cout << "Completed! Generated video: " << OUTPUT_FILENAME << " with " << frame_count << " frames." << std::endl;
  return 0;
}

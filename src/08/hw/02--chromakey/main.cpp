#include <algorithm>
#include <cmath>
#include <iostream>
#include <opencv2/opencv.hpp>

#include "../../utils/opencv.hpp"

#define BACKGROUND_FILENAME "./assets/dome.jpg"
#define FOREGROUND1_FILENAME "./assets/girls.jpg"
#define FOREGROUND2_FILENAME "./assets/logo.jpg"
#define OUTPUT_FILENAME "./out/output.mp4"

int main(int argc, char *argv[], char *envp[])
{

  auto bg = cv::imread(BACKGROUND_FILENAME);
  if (bg.empty())
  {
    std::cerr << "Failed to load background image: " << BACKGROUND_FILENAME << std::endl;
    return -1;
  }

  auto fg1 = cv::imread(FOREGROUND1_FILENAME);
  if (fg1.empty())
  {
    std::cerr << "Failed to load foreground 1: " << FOREGROUND1_FILENAME << std::endl;
    return -1;
  }

  auto fg2 = cv::imread(FOREGROUND2_FILENAME);
  if (fg2.empty())
  {
    std::cerr << "Failed to load foreground 2: " << FOREGROUND2_FILENAME << std::endl;
    return -1;
  }

  int frame_width = 900;
  int frame_height = 600;
  cv::Size frame_size(frame_width, frame_height);

  double fps = 30.0;
  int total_frames = 300;
  auto fourcc = cv::VideoWriter::fourcc('m', 'p', '4', 'v');
  cv::VideoWriter writer(OUTPUT_FILENAME, fourcc, fps, frame_size);
  if (!writer.isOpened())
  {
    std::cerr << "Failed to open VideoWriter: " << OUTPUT_FILENAME << std::endl;
    return -1;
  }

  auto fg1_hsv = cv::Mat();
  cv::cvtColor(fg1, fg1_hsv, cv::COLOR_BGR2HSV);
  auto fg2_hsv = cv::Mat();
  cv::cvtColor(fg2, fg2_hsv, cv::COLOR_BGR2HSV);

  cv::Scalar lower_green(35, 40, 40);
  cv::Scalar upper_green(85, 255, 255);

  cv::Mat green_mask1, green_mask2;
  cv::inRange(fg1_hsv, lower_green, upper_green, green_mask1);
  cv::inRange(fg2_hsv, lower_green, upper_green, green_mask2);

  cv::Mat mask1 = ~green_mask1;
  cv::Mat mask2 = ~green_mask2;

  int frame_count = 0;
  std::cout << "Starting video generation loop..." << std::endl;

  while (frame_count < total_frames)
  {
    auto progress = static_cast<double>(frame_count) / (total_frames - 1);

    int bg_x = std::round(300.0 * progress);
    int bg_y = std::round(202.0 * progress);
    cv::Rect crop_window(bg_x, bg_y, frame_width, frame_height);
    cv::Mat frame = bg(crop_window).clone();

    auto ty1 = 600.0 - 511.0 * (progress / 0.5);
    if (ty1 < 89.0)
    {
      ty1 = 89.0;
    }
    auto tx1 = 0.0;
    cv::Mat M1 = (cv::Mat_<double>(2, 3) << 1.0, 0.0, tx1, 0.0, 1.0, ty1);

    cv::Mat warped_fg1, warped_mask1;
    cv::warpAffine(fg1, warped_fg1, M1, frame_size, cv::INTER_LINEAR, cv::BORDER_CONSTANT, cv::Scalar(0, 0, 0));
    cv::warpAffine(mask1, warped_mask1, M1, frame_size, cv::INTER_NEAREST, cv::BORDER_CONSTANT, cv::Scalar(0));

    warped_fg1.copyTo(frame, warped_mask1);

    auto scale = 0.3;
    auto theta = progress * 360.0;
    auto angle_rad = theta * CV_PI / 180.0;
    auto cos_a = std::cos(angle_rad);
    auto sin_a = std::sin(angle_rad);

    auto X_c = 450.0 + 250.0 * std::sin(2.0 * CV_PI * 2.0 * progress);
    auto Y_c = 300.0 + 200.0 * std::cos(2.0 * CV_PI * 3.0 * progress);

    auto orig_cx = fg2.cols / 2.0;
    auto orig_cy = fg2.rows / 2.0;

    auto M2 = static_cast<cv::Mat>(cv::Mat::zeros(2, 3, CV_64F));
    M2.at<double>(0, 0) = scale * cos_a;
    M2.at<double>(0, 1) = -scale * sin_a;
    M2.at<double>(0, 2) = X_c - scale * cos_a * orig_cx + scale * sin_a * orig_cy;
    M2.at<double>(1, 0) = scale * sin_a;
    M2.at<double>(1, 1) = scale * cos_a;
    M2.at<double>(1, 2) = Y_c - scale * sin_a * orig_cx - scale * cos_a * orig_cy;

    auto warped_fg2 = cv::Mat();
    auto warped_mask2 = cv::Mat();
    cv::warpAffine(fg2, warped_fg2, M2, frame_size, cv::INTER_LINEAR, cv::BORDER_CONSTANT, cv::Scalar(0, 0, 0));
    cv::warpAffine(mask2, warped_mask2, M2, frame_size, cv::INTER_NEAREST, cv::BORDER_CONSTANT, cv::Scalar(0));

    warped_fg2.copyTo(frame, warped_mask2);

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

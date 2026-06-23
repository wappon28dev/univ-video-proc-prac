#include <algorithm>
#include <cmath>
#include <iostream>
#include <vector>
#include <opencv2/opencv.hpp>

#include "../../utils/opencv.hpp"

#define VIDEO_FILENAME "./assets/water1.mov"
#define SHIP_FILENAME "./assets/ship.jpg"
#define OUTPUT_FILENAME "./out/output.mp4"

int main(int argc, char *argv[], char *envp[])
{
  auto cap = cv::VideoCapture(VIDEO_FILENAME);
  if (!cap.isOpened())
  {
    std::cerr << "Failed to open video: " << VIDEO_FILENAME << std::endl;
    return -1;
  }

  auto ship = cv::imread(SHIP_FILENAME);
  if (ship.empty())
  {
    std::cerr << "Failed to load ship: " << SHIP_FILENAME << std::endl;
    return -1;
  }

  cv::Mat ship_hsv;
  cv::cvtColor(ship, ship_hsv, cv::COLOR_BGR2HSV);
  cv::Scalar lower_green(35, 40, 40);
  cv::Scalar upper_green(85, 255, 255);
  cv::Mat green_mask;
  cv::inRange(ship_hsv, lower_green, upper_green, green_mask);
  cv::Mat ship_mask = ~green_mask;

  int frame_width = static_cast<int>(cap.get(cv::CAP_PROP_FRAME_WIDTH));
  int frame_height = static_cast<int>(cap.get(cv::CAP_PROP_FRAME_HEIGHT));
  cv::Size frame_size(frame_width, frame_height);

  double fps = cap.get(cv::CAP_PROP_FPS);
  if (fps <= 0.0)
  {
    fps = 30.0;
  }

  auto fourcc = cv::VideoWriter::fourcc('m', 'p', '4', 'v');
  cv::VideoWriter writer(OUTPUT_FILENAME, fourcc, fps, frame_size);
  if (!writer.isOpened())
  {
    std::cerr << "Failed to open VideoWriter: " << OUTPUT_FILENAME << std::endl;
    return -1;
  }

  cv::Mat frame_prev;
  cap >> frame_prev;
  if (frame_prev.empty())
  {
    return -1;
  }

  cv::Mat gray_prev;
  cv::cvtColor(frame_prev, gray_prev, cv::COLOR_BGR2GRAY);

  double x = 130.0;
  double y = 190.0;
  double vx = 0.0;
  double vy = 0.0;
  double theta = 0.0;
  double scale = 0.5;
  double alpha_vel = 0.15;

  double orig_cx = ship.cols / 2.0;
  double orig_cy = ship.rows / 2.0;

  cv::Mat first_frame_out = frame_prev.clone();
  double angle_rad = theta * 3.14159265358979323846 / 180.0;
  double cos_a = std::cos(angle_rad);
  double sin_a = std::sin(angle_rad);

  cv::Mat M = cv::Mat::zeros(2, 3, CV_64F);
  M.at<double>(0, 0) = scale * cos_a;
  M.at<double>(0, 1) = -scale * sin_a;
  M.at<double>(0, 2) = x - scale * cos_a * orig_cx + scale * sin_a * orig_cy;
  M.at<double>(1, 0) = scale * sin_a;
  M.at<double>(1, 1) = scale * cos_a;
  M.at<double>(1, 2) = y - scale * sin_a * orig_cx - scale * cos_a * orig_cy;

  cv::Mat warped_ship;
  cv::Mat warped_mask;
  cv::warpAffine(ship, warped_ship, M, frame_size, cv::INTER_LINEAR, cv::BORDER_CONSTANT, cv::Scalar(0, 0, 0));
  cv::warpAffine(ship_mask, warped_mask, M, frame_size, cv::INTER_NEAREST, cv::BORDER_CONSTANT, cv::Scalar(0));
  warped_ship.copyTo(first_frame_out, warped_mask);

  mat_util::show("Composition Preview", first_frame_out);
  writer.write(first_frame_out);

  while (true)
  {
    cv::Mat frame;
    cap >> frame;
    if (frame.empty())
    {
      break;
    }

    cv::Mat gray;
    cv::cvtColor(frame, gray, cv::COLOR_BGR2GRAY);

    std::vector<cv::Point2f> priorFeature;
    std::vector<int> offsets = {-15, -5, 5, 15};
    for (int dy : offsets)
    {
      for (int dx : offsets)
      {
        priorFeature.push_back(cv::Point2f(x + dx, y + dy));
      }
    }

    std::vector<cv::Point2f> points_next;
    std::vector<uchar> status;
    std::vector<float> errors;

    cv::calcOpticalFlowPyrLK(
        gray_prev,
        gray,
        priorFeature,
        points_next,
        status,
        errors,
        cv::Size(15, 15),
        3,
        cv::TermCriteria(cv::TermCriteria::MAX_ITER | cv::TermCriteria::EPS, 30, 0.01));

    std::vector<cv::Point2f> displacements;
    for (size_t i = 0; i < priorFeature.size(); ++i)
    {
      if (status[i] == 1)
      {
        cv::Point2f diff = points_next[i] - priorFeature[i];
        double dist = std::hypot(diff.x, diff.y);
        if (dist > 0.05 && dist < 30.0)
        {
          displacements.push_back(diff);
        }
      }
    }

    if (!displacements.empty())
    {
      double sum_dx = 0.0;
      double sum_dy = 0.0;
      for (const auto &d : displacements)
      {
        sum_dx += d.x;
        sum_dy += d.y;
      }
      double mean_dx = sum_dx / displacements.size();
      double mean_dy = sum_dy / displacements.size();

      vx = alpha_vel * mean_dx + (1.0 - alpha_vel) * vx;
      vy = alpha_vel * mean_dy + (1.0 - alpha_vel) * vy;
    }

    x += vx;
    y += vy;

    if (std::hypot(vx, vy) > 0.01)
    {
      double target_theta = std::atan2(vy, vx) * 180.0 / 3.14159265358979323846;
      theta = 0.15 * target_theta + 0.85 * theta;
    }

    cv::Mat output_frame = frame.clone();
    angle_rad = theta * 3.14159265358979323846 / 180.0;
    cos_a = std::cos(angle_rad);
    sin_a = std::sin(angle_rad);

    M = cv::Mat::zeros(2, 3, CV_64F);
    M.at<double>(0, 0) = scale * cos_a;
    M.at<double>(0, 1) = -scale * sin_a;
    M.at<double>(0, 2) = x - scale * cos_a * orig_cx + scale * sin_a * orig_cy;
    M.at<double>(1, 0) = scale * sin_a;
    M.at<double>(1, 1) = scale * cos_a;
    M.at<double>(1, 2) = y - scale * sin_a * orig_cx - scale * cos_a * orig_cy;

    cv::warpAffine(ship, warped_ship, M, frame_size, cv::INTER_LINEAR, cv::BORDER_CONSTANT, cv::Scalar(0, 0, 0));
    cv::warpAffine(ship_mask, warped_mask, M, frame_size, cv::INTER_NEAREST, cv::BORDER_CONSTANT, cv::Scalar(0));
    warped_ship.copyTo(output_frame, warped_mask);

    mat_util::show("Composition Preview", output_frame);
    writer.write(output_frame);

    int key = cv::waitKey(33);
    if (key == 'q')
    {
      break;
    }

    gray_prev = gray;
  }

  cap.release();
  writer.release();
  cv::destroyAllWindows();

  return 0;
}

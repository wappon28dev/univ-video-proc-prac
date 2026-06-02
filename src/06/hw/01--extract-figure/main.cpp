#include <iostream>
#include <opencv2/opencv.hpp>
#include <ranges>

#include "../../utils/opencv.hpp"

#define INPUT_FILENAME "./assets/kadai.png"
#define AREA_THRESHOLD 50000
#define OUTPUT_FILENAME "./out/extracted.png"

using Contour = std::vector<cv::Point>;

const auto c_black = cv::Scalar(0, 0, 0);
const auto c_white = cv::Scalar(255, 255, 255);

int main(int argc, char *argv[], char *envp[])
{
  auto input = cv::imread(INPUT_FILENAME);
  if (input.empty())
  {
    std::cerr << "Failed to load image: " << INPUT_FILENAME << std::endl;
    return -1;
  }

  auto input_bin = cv::Mat();
  cv::cvtColor(input, input_bin, cv::COLOR_BGR2GRAY);
  cv::threshold(input_bin, input_bin, 128, 255, cv::THRESH_BINARY);

  auto contours = std::vector<Contour>();
  cv::findContours(input_bin.clone(), contours, cv::RETR_LIST, cv::CHAIN_APPROX_NONE);
  auto pred = [](const Contour &contour) { return cv::contourArea(contour) >= AREA_THRESHOLD; };
  auto filtered_contours = contours | std::views::filter(pred);

  auto output = input.clone();
  for (const auto &contour : filtered_contours)
  {
    cv::drawContours(output, std::vector<Contour>{contour}, -1, c_white, 4);
  }

  mat_util::show("output", output);
  cv::waitKey(0);

  cv::imwrite(OUTPUT_FILENAME, output);

  return 0;
}

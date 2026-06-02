#include <iostream>
#include <opencv2/opencv.hpp>
#include <ranges>

#include "../../utils/opencv.hpp"

#define INPUT_FILENAME "./assets/kadai.png"
#define BINARY_FILENAME "./out/binary.png"
#define OUTPUT_FILENAME "./out/extracted.png"

using Contour = std::vector<cv::Point>;

const auto c_black = cv::Scalar(0, 0, 0);
const auto c_white = cv::Scalar(255, 255, 255);

cv::Mat preprocess(const cv::Mat &input)
{
  auto input_bin = cv::Mat();
  cv::cvtColor(input, input_bin, cv::COLOR_BGR2GRAY);
  cv::threshold(input_bin, input_bin, 128, 255, cv::THRESH_BINARY);

  auto kernel = cv::getStructuringElement(cv::MORPH_CLOSE, cv::Size(4, 4));

  cv::dilate(input_bin, input_bin, kernel);
  cv::dilate(input_bin, input_bin, kernel);
  cv::dilate(input_bin, input_bin, kernel);
  cv::erode(input_bin, input_bin, kernel);

  return input_bin;
}

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

  input_bin = preprocess(input);
  cv::imwrite(BINARY_FILENAME, input_bin);

  auto contours = std::vector<Contour>();
  cv::findContours(input_bin.clone(), contours, cv::RETR_LIST, cv::CHAIN_APPROX_NONE);
  auto pred = [](const Contour &contour) {
    auto area = cv::contourArea(contour);
    auto perimeter = cv::arcLength(contour, true);
    auto circularity = 4 * M_PI * area / (perimeter * perimeter);

    std::cout << "area: " << area << "\t perimeter: " << perimeter << "\t circularity: " << circularity << std::endl;

    return circularity > 0.5;
  };
  auto filtered_contours = contours | std::views::filter(pred);

  auto output = input.clone();
  for (const auto &contour : filtered_contours)
  {
    cv::drawContours(output, std::vector<Contour>{contour}, -1, c_white, 4);
  }

  cv::imwrite(OUTPUT_FILENAME, output);

  return 0;
}

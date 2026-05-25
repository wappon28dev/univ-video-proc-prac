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

typedef std::vector<std::vector<float>> LookupTable;
LookupTable get_lookup_table()
{
  auto lookupTable = LookupTable(3, std::vector<float>(256, 0));

  for (int i = 0; i < 256; i++)
  {
    lookupTable[0][i] = i;
    lookupTable[1][i] = i;
    if (i < 64)
    {

      lookupTable[2][i] = 0;
    }
    else if (i < 128)
    {

      lookupTable[2][i] = 85;
    }
    else if (i < 196)
    {

      lookupTable[2][i] = 170;
    }
    else
    {

      lookupTable[2][i] = 255;
    }
  }
  return lookupTable;
}

cv::Mat to_animate(const cv::Mat frame, const LookupTable &lookupTable)
{
  auto frame_out = cv::Mat(frame.size(), frame.type());
  cv::bilateralFilter(frame, frame_out, 9, 75, 75);

  cv::cvtColor(frame_out, frame_out, cv::COLOR_BGR2HSV);

  auto lt_h = lookupTable[0].data();
  auto lt_s = lookupTable[1].data();
  auto lt_v = lookupTable[2].data();

  frame_out.forEach<cv::Vec3b>([&](cv::Vec3b &pixel, const int *position) -> void {
    pixel[0] = static_cast<uchar>(lt_h[pixel[0]]);
    pixel[1] = static_cast<uchar>(lt_s[pixel[1]]);
    pixel[2] = static_cast<uchar>(lt_v[pixel[2]]);
  });

  cv::cvtColor(frame_out, frame_out, cv::COLOR_HSV2BGR);
  return frame_out;
}

cv::Mat process(const cv::Mat &frame, const LookupTable &lookupTable)
{
  auto anime_frame = to_animate(frame, lookupTable);
  auto frame_out = cv::Mat();
  auto edge = cv::Mat();

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
    auto kernel = cv::Mat(cv::Size(3, 3), CV_32F, fdata_laplacian);
    cv::filter2D(frame_out, frame_out, CV_32F, kernel);
    cv::convertScaleAbs(frame_out, edge);
  }

  // ２値化
  cv::threshold(edge, frame_out, 50, 255, cv::THRESH_BINARY_INV);

  cv::cvtColor(frame_out, frame_out, cv::COLOR_GRAY2BGR);
  cv::bitwise_and(anime_frame, frame_out, frame_out);

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
  const auto lookupTable = get_lookup_table();
  while (true)
  {
    capture >> frame;
    if (frame.empty())
    {
      std::cerr << "Error: Empty frame captured." << std::endl;
      break;
    }

    auto frame_out = process(frame, lookupTable);

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

#include <iostream>
#include <opencv2/opencv.hpp>

constexpr auto OUTPUT_FILENAME = "./out/output.mp4";

int main()
{
  cv::VideoCapture cap(0);
  if (!cap.isOpened())
  {
    std::cerr << "Error: Camera could not be opened." << std::endl;
    return -1;
  }

  cv::Mat frame;
  cv::VideoWriter writer;
  auto fourcc = cv::VideoWriter::fourcc('m', 'p', '4', 'v');

  while (true)
  {
    cap >> frame;
    if (frame.empty())
    {
      break;
    }

    int half_width = frame.cols / 2;
    
    // Crop frame to an even width to ensure left and right halves are identical in size
    cv::Mat cropped_frame = frame(cv::Rect(0, 0, 2 * half_width, frame.rows));
    cv::Mat output = cropped_frame.clone();

    cv::Rect left_roi(0, 0, half_width, frame.rows);
    cv::Rect right_roi(half_width, 0, half_width, frame.rows);

    // Get the flipped version of the left half
    cv::Mat left_flipped;
    cv::flip(cropped_frame(left_roi), left_flipped, 1);

    // Paste the flipped left half into the right half of output
    left_flipped.copyTo(output(right_roi));

    // Initialize VideoWriter dynamically on the first frame
    if (!writer.isOpened())
    {
      double fps = 30.0;
      writer.open(OUTPUT_FILENAME, fourcc, fps, output.size());
      if (!writer.isOpened())
      {
        std::cerr << "Error: Could not open VideoWriter: " << OUTPUT_FILENAME << std::endl;
      }
    }

    // Write the split-view frame to the MP4 file
    if (writer.isOpened())
    {
      writer.write(output);
    }

    // Show symmetric frame in one window, and original frame in another window
    cv::imshow("Split View (Left: Normal, Right: Flipped Left)", output);
    cv::imshow("Original Camera", frame);

    if (cv::waitKey(1) >= 0)
    {
      break;
    }
  }

  writer.release();
  return 0;
}

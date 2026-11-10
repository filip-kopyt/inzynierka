#include "video_display.h"

void VideoDisplay::work(std::pair<cv::Mat, cv::Mat> &&input) {
  auto [frame1, frame2] = input;
  if (frame1.empty() || frame2.empty())
    return;

  cv::Mat merged_frames, resized_frames;
  cv::hconcat(std::array{frame1, frame2}, merged_frames);
  cv::resize(merged_frames, resized_frames, cv::Size(), 0.5, 0.5);
  cv::imshow("frames", resized_frames);
  cv::waitKey(1);
}

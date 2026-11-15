#include "video_display.h"
#include <opencv2/core.hpp>
#include <opencv2/core/base.hpp>
#include <opencv2/core/hal/interface.h>

void VideoDisplay::work(cv::Mat &&input) {
  cv::Mat resized_frame, normalized_frame;
  cv::normalize(input, normalized_frame, 0, 255, cv::NORM_MINMAX, CV_8UC1);
  //cv::resize(normalized_frame, resized_frame, cv::Size(), 0.5, 0.5);
  cv::imshow("frame", normalized_frame);
  cv::waitKey(1);
}

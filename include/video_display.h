#pragma once

#include <memory>
#include <opencv2/core.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/opencv.hpp>

#include "config.h"
#include "pipeline_element.hpp"

class VideoDisplay : public PipelineElement<std::pair<cv::Mat, cv::Mat>, void> {
public:
  VideoDisplay(auto &in) : PipelineElement(in) {}

  void work(std::pair<cv::Mat, cv::Mat> &&input) {
    auto [frame1, frame2] = input;
    if (frame1.empty() || frame2.empty())
      return;

    cv::Mat merged_frames, resized_frames;
    cv::hconcat(std::array{frame1, frame2}, merged_frames);
    cv::resize(merged_frames, resized_frames, cv::Size(), 0.5, 0.5);
    // std::cout << "test\n";
    cv::imshow("frames", resized_frames);
    cv::waitKey(1);
  }
};
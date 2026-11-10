#pragma once

#include <opencv2/core.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/opencv.hpp>

#include "pipeline_element.hpp"

class VideoDisplay : public PipelineElement<std::pair<cv::Mat, cv::Mat>, void> {
public:
  VideoDisplay(auto &in) : PipelineElement(in) {}

  void work(std::pair<cv::Mat, cv::Mat> &&input);
};

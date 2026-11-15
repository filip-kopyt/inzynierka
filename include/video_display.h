#pragma once

#include <opencv2/core.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/opencv.hpp>

#include "pipeline_element.hpp"

class VideoDisplay : public PipelineElement<cv::Mat, void> {
public:
  VideoDisplay(auto &in) : PipelineElement(in) {}

  virtual void work(cv::Mat &&input) override;
};

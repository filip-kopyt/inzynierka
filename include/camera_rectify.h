#pragma once

#include <filesystem>
#include <opencv2/core/mat.hpp>
#include <opencv2/core/persistence.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/stereo.hpp>
#include <utility>

#include "config.h"
#include "pipeline_element.hpp"

class CameraRectify : public PipelineElement<std::pair<cv::Mat, cv::Mat>,
                                             std::pair<cv::Mat, cv::Mat>> {
private:
  cv::Mat map_x_left, map_y_left;
  cv::Mat map_x_right, map_y_right;

public:
  CameraRectify(Slot<PipelineElement::Input> &input,
                std::filesystem::path calibration_file);

  virtual PipelineElement::Output
  work(PipelineElement::Input &&frames) override;
};
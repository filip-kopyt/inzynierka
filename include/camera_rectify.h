#pragma once

#include <opencv2/core/mat.hpp>
#include <utility>

#include "pipeline_element.hpp"

class CameraRectify : public PipelineElement<std::pair<size_t, cv::Mat>,
                                             std::pair<size_t, cv::Mat>> {
private:
  cv::Mat map1, map2;

public:
  CameraRectify(Slot<std::pair<size_t, cv::Mat>> &input, cv::Mat map1,
                cv::Mat map2);

  virtual std::pair<size_t, cv::Mat>
  work(std::pair<size_t, cv::Mat> &&input) override;

  static std::pair<std::unique_ptr<CameraRectify>,
                   std::unique_ptr<CameraRectify>>
  setup_rectifiers(Slot<std::pair<size_t, cv::Mat>> &cam1_input,
                   Slot<std::pair<size_t, cv::Mat>> &cam2_input,
                   std::string path_to_params);
};
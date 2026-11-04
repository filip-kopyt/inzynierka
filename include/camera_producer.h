#pragma once

#include <cstddef>
#include <memory>
#include <opencv2/core.hpp>
#include <utility>
#include <vector>

#include "pipeline_element.hpp"

class CameraProducer : public PipelineElement<void, std::pair<size_t, cv::Mat>> {
private:
  int _counter = 0;
  int _cam_fd;
  void *_cam_buf;

public:
  CameraProducer(int camera_index);
  CameraProducer(const CameraProducer &) = delete;
  CameraProducer &operator=(const CameraProducer &) = delete;

  virtual std::pair<size_t, cv::Mat> work() override;
  void update_exposure(int exposure);

  ~CameraProducer();
};
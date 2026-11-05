#pragma once

#include <gpiod.hpp>
#include <opencv2/core/mat.hpp>
#include <thread>
#include <utility>

#include "pipeline_element.hpp"

using FileDescriptor = int;
using BufferPtr = void *;

class StereoCamera : public PipelineElement<void, std::pair<cv::Mat, cv::Mat>> {
private:
  struct CameraMeta {
    std::thread collection_thread;
    Slot<cv::Mat> sink;
    BufferPtr buffer;
    FileDescriptor fd;
    bool running = true;

    void loop();
  };

  CameraMeta left, right;

  gpiod::chip _gpio_chip{"/dev/gpiochip0"};
  gpiod::line_request _gpio_line;

  void disable_pwm();
  void enable_pwm();

public:
  StereoCamera();

  void update_exposure(int value);
  virtual std::pair<cv::Mat, cv::Mat> work() override;
};

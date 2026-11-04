#pragma once

#include <MiniPID.h>
#include <opencv2/core.hpp>
#include <utility>

#include "camera_producer.h"
#include "config.h"

class ExposureRegulator : public PipelineElement<std::pair<cv::Mat, cv::Mat>, void> {
private:
  MiniPID _pid;
  CameraProducer &_cam1, &_cam2;

  double _exposure = 400;

public:
  ExposureRegulator(Slot<std::pair<cv::Mat, cv::Mat>> &input,
                    CameraProducer &cam1, CameraProducer &cam2, double p,
                    double i, double d)
      : _cam1(cam1), _cam2(cam2), _pid(p, i, d), PipelineElement(input) {
    _pid.setOutputLimits(-25, 25);
    _pid.setSetpointRange(50);
    _pid.setOutputFilter(0.1);
    _pid.setOutputRampRate(1);
    //_pid.setMaxIOutput(50);
    _pid.setSetpoint(to_pidspace(CAM_TARGET_BRIGHTNESS));
  }

  virtual void work(std::pair<cv::Mat, cv::Mat> &&frames) override {
    auto [frame1, frame2] = frames;
    cv::Mat merged;
    cv::Scalar mean, stddev;

    cv::hconcat(std::array{frame1, frame2}, merged);
    cv::meanStdDev(merged, mean, stddev);

    auto actual = to_pidspace(mean.val[0]);
    _exposure += _pid.getOutput(actual);
    _exposure = std::clamp(_exposure, 1.0, 800.0);

    _cam1.update_exposure(_exposure);
    _cam2.update_exposure(_exposure);
  }

  int to_pidspace(int color) { return color * 800 / 256; }
};
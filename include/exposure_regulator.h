#pragma once

#include <MiniPID.h>
#include <opencv2/core.hpp>
#include <utility>

#include "stereo_camera.h"

class ExposureRegulator
    : public PipelineElement<std::pair<cv::Mat, cv::Mat>, void> {
private:
  MiniPID _pid;
  StereoCamera &_cams;

  double _exposure = 400;

public:
  ExposureRegulator(Slot<std::pair<cv::Mat, cv::Mat>> &input,
                    StereoCamera &cams, double p, double i, double d);

  virtual void work(std::pair<cv::Mat, cv::Mat> &&frames) override;

  static int to_pidspace(int color);

  virtual void stop() override;
};

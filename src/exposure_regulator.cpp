#include "exposure_regulator.h"

#include "config.h"

ExposureRegulator::ExposureRegulator(Slot<std::pair<cv::Mat, cv::Mat>> &input,
                                     StereoCamera &cams, double p, double i,
                                     double d)
    : _cams(cams), _pid(p, i, d), PipelineElement(input) {
  _pid.setOutputLimits(-25, 25);
  _pid.setSetpointRange(50);
  _pid.setOutputFilter(0.1);
  _pid.setOutputRampRate(1);
  //_pid.setMaxIOutput(50);
  _pid.setSetpoint(to_pidspace(CAM_TARGET_BRIGHTNESS));
}

void ExposureRegulator::work(std::pair<cv::Mat, cv::Mat> &&frames) {
  auto [frame1, frame2] = frames;
  cv::Mat merged;
  cv::Scalar mean, stddev;

  cv::hconcat(std::array{frame1, frame2}, merged);
  cv::meanStdDev(merged, mean, stddev);

  auto actual = to_pidspace(mean.val[0]);
  _exposure += _pid.getOutput(actual);
  _exposure = std::clamp(_exposure, 1.0, 800.0);

  _cams.update_exposure(_exposure);
}

int ExposureRegulator::to_pidspace(int color) { return color * 800 / 256; }

void ExposureRegulator::stop() {
  std::cout << "Current exposure value = " << _exposure << "\n";
  PipelineElement::stop();
}

#include "disparity_calculation.h"

DisparityCalculation::DisparityCalculation(
    cv::Ptr<cv::StereoMatcher> matcher,
    Slot<std::pair<cv::Mat, cv::Mat>> &input)
    : PipelineElement(input), _left_matcher(matcher) {
  _right_matcher = cv::ximgproc::createRightMatcher(_left_matcher);
}

cv::Mat DisparityCalculation::work(std::pair<cv::Mat, cv::Mat> &&input) {
  auto [left, right] = input;
  cv::Mat dispL, dispR, filtered_disp;
  _left_matcher->compute(left, right, dispL);

  if (!use_wls_filter) {
    return dispL;
  }

  _right_matcher->compute(right, left, dispR);
  _wls_filter->filter(dispL, left, filtered_disp, dispR, cv::Rect(), right);
  return filtered_disp;
}

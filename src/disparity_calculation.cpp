#include "disparity_calculation.h"
#include <opencv2/core/types.hpp>
#include <opencv2/imgproc.hpp>

DisparityCalculation::DisparityCalculation(
    cv::Ptr<cv::StereoMatcher> matcher,
    Slot<std::pair<cv::Mat, cv::Mat>> input)
    : PipelineElement(std::move(input)), _left_matcher(matcher) {
  _right_matcher = cv::ximgproc::createRightMatcher(_left_matcher);
}

cv::Mat DisparityCalculation::work(std::pair<cv::Mat, cv::Mat> &&input) {
  auto [left, right] = input;
  cv::Mat left_scaled, right_scaled;
  cv::resize(left, left_scaled, cv::Size(), 0.5, 0.5);
  cv::resize(right, right_scaled, cv::Size(), 0.5, 0.5);

  cv::Mat dispL, dispR, filtered_disp;
  _left_matcher->compute(left_scaled, right_scaled, dispL);

  if (!use_wls_filter) {
    return dispL;
  }

  _right_matcher->compute(right, left, dispR);
  _wls_filter->filter(dispL, left, filtered_disp, dispR, cv::Rect(), right);
  return filtered_disp;
}

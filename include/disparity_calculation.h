#pragma once

#include <opencv2/calib3d.hpp>
#include <opencv2/core/cvstd_wrapper.hpp>
#include <opencv2/core/mat.hpp>
#include <opencv2/ximgproc/disparity_filter.hpp>
#include <utility>

#include "pipeline_element.hpp"

class DisparityCalculation
    : public PipelineElement<std::pair<cv::Mat, cv::Mat>, cv::Mat> {
private:
  cv::Ptr<cv::StereoMatcher> _left_matcher, _right_matcher;
  cv::Ptr<cv::ximgproc::DisparityWLSFilter> _wls_filter;
  
public:
  bool use_wls_filter = false;

  DisparityCalculation(cv::Ptr<cv::StereoMatcher> matcher,
                       Slot<std::pair<cv::Mat, cv::Mat>> &input)
      : _left_matcher(matcher), PipelineElement(input) {
        _right_matcher = cv::ximgproc::createRightMatcher(_left_matcher);
      }

  virtual cv::Mat work(std::pair<cv::Mat, cv::Mat> &&input) override {
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
};

#if 0
  cv::Ptr<cv::cudaStereoConstantSpaceBP> matcher = cv::cuda::createStereoConstantSpaceBP()
#endif
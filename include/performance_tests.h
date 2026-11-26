#pragma once

#include <cstddef>
#include <numeric>
#include <opencv2/cudastereo.hpp>
#include <ratio>
#include <utility>
#include <vector>

extern const size_t RUNS;

std::vector<std::pair<cv::Mat, cv::Mat>> load_dataset();

template <typename T>
std::pair<float, float>
test_matcher(T matcher, cv::Ptr<cv::cuda::DisparityBilateralFilter> filter,
             const std::vector<std::pair<cv::Mat, cv::Mat>> &dataset) {
  std::vector<float> times;
  std::vector<float> partial_stds;

  for (size_t run = 0; run < RUNS; run++) {
    for (const auto &[left, right] : dataset) {
      cv::cuda::Stream stream;
      cv::cuda::GpuMat left_gpu, right_gpu, disp_gpu, disp_filtered_gpu;

      left_gpu.upload(left);
      right_gpu.upload(right);

      auto start_time = std::chrono::high_resolution_clock::now();
      matcher->compute(left_gpu, right_gpu, disp_gpu, stream);
      filter->apply(disp_gpu, left_gpu, disp_filtered_gpu, stream);
      stream.waitForCompletion();
      std::chrono::duration<float, std::milli> time =
          std::chrono::high_resolution_clock::now() - start_time;
      times.push_back(time.count());
    }
  }

  auto average = std::reduce(times.begin(), times.end()) / times.size();
  for (const auto &time : times) {
    partial_stds.push_back(std::pow(time - average, 2));
  }
  auto std = std::sqrt(std::reduce(partial_stds.begin(), partial_stds.end()) /
                       partial_stds.size());
  return {average, std};
}

template <typename T>
std::pair<float, float>
test_matcher(T matcher,
             const std::vector<std::pair<cv::Mat, cv::Mat>> &dataset) {
  std::vector<float> times;
  std::vector<float> partial_stds;

  for (size_t run = 0; run < RUNS; run++) {
    for (const auto &[left, right] : dataset) {
      // cv::cuda::Stream stream;
      cv::cuda::GpuMat left_gpu, right_gpu, disp_gpu, disp_filtered_gpu;

      left_gpu.upload(left);
      right_gpu.upload(right);

      auto start_time = std::chrono::high_resolution_clock::now();
      matcher->compute(left_gpu, right_gpu, disp_gpu); //, stream);
      // filter->apply(disp_gpu, left_gpu, disp_filtered_gpu, stream);
      // stream.waitForCompletion();
      std::chrono::duration<float, std::milli> time =
          std::chrono::high_resolution_clock::now() - start_time;
      times.push_back(time.count());
    }
  }

  auto average = std::reduce(times.begin(), times.end()) / times.size();
  for (const auto &time : times) {
    partial_stds.push_back(std::pow(time - average, 2));
  }
  auto std = std::sqrt(std::reduce(partial_stds.begin(), partial_stds.end()) /
                       partial_stds.size());
  return {average, std};
}
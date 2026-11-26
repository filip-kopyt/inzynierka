#include <iomanip>
#include <ios>
#include <iostream>
#include <opencv2/cudastereo.hpp>
#include <opencv2/stereo.hpp>
#include <signal.h>

#include "camera_rectify.h"
#include "cuda_algs.h"
#include "disparity_calculation.h"
#include "exposure_regulator.h"
#include "file_stream.h"
#include "frame_writer.h"
#include "performance_tests.h"
#include "stereo_camera.h"
#include "video_display.h"

static bool running = true;

void signal_handler(int) {
  std::cout << "stopping\n";
  running = false;
}

void run(std::vector<PipelineElementBase *> parts) {
  std::vector<std::thread> threads;
  signal(SIGINT, &signal_handler);

  std::cout << "Starting\n";

  for (const auto &part : parts) {
    threads.push_back(part->start());
  }

  while (running)
    std::this_thread::yield();

  for (const auto &part : parts) {
    part->stop();
  }

  for (auto &thread : threads) {
    thread.join();
  }
}

// int main() {
//   int ndisp, iters, levels, nr_plane;
//   cv::cuda::StereoConstantSpaceBP::estimateRecommendedParams(1600, 1300,
//   ndisp, iters, levels, nr_plane); std::cout << ndisp << " " << iters << " "
//   << levels << " " << nr_plane << "\n";
// }

//#define PERF_TEST
#ifndef PERF_TEST

int main() {
  auto matcher = cv::StereoSGBM::create(0, 128, 1, 10, 40, 2, 16, 15, 50, 1);

  auto file_stream = FileStream("/home/filip-kopyt/jetson-data/frames0");
  auto rectify = CameraRectify(file_stream.output(), "calibration.ext");
  auto disparity = DisparityCalculation(matcher, rectify.output());
  auto display = VideoDisplay(disparity.output());

  run({&file_stream, &rectify, &disparity, &display});
}

#else

int main() {
  auto dataset = load_dataset();

  std::cout << std::fixed << std::setprecision(2);
  {
    auto matcher = cv::cuda::createStereoSGM(0, 128, 10, 120, 5,
                                             cv::cuda::StereoSGM::MODE_HH4);
    auto filter = cv::cuda::createDisparityBilateralFilter();
    auto [average, std] = test_matcher(matcher, filter, dataset);
    std::cout << "SGM_HH4+filter  avg=" << std::setw(6) << average
              << "ms std=" << std::setw(6) << std << "ms\n";
  }
  {
    auto matcher = cv::cuda::createStereoSGM(0, 128, 10, 120, 5,
                                             cv::cuda::StereoSGM::MODE_HH4);
    // auto filter = cv::cuda::createDisparityBilateralFilter();
    auto [average, std] = test_matcher(matcher, dataset);
    std::cout << "SGM_HH4         avg=" << std::setw(6) << average
              << "ms std=" << std::setw(6) << std << "ms\n";
  }
  {
    auto matcher = cv::cuda::createStereoSGM(0, 128, 10, 120, 5,
                                             cv::cuda::StereoSGM::MODE_HH);
    auto filter = cv::cuda::createDisparityBilateralFilter();
    auto [average, std] = test_matcher(matcher, filter, dataset);
    std::cout << "SGM_HH+filter   avg=" << std::setw(6) << average
              << "ms std=" << std::setw(6) << std << "ms\n";
  }
  {
    auto matcher = cv::cuda::createStereoSGM(0, 128, 10, 120, 5,
                                             cv::cuda::StereoSGM::MODE_HH);
    // auto filter = cv::cuda::createDisparityBilateralFilter();
    auto [average, std] = test_matcher(matcher, dataset);
    std::cout << "SGM_HH          avg=" << std::setw(6) << average
              << "ms std=" << std::setw(6) << std << "ms\n";
  }
  {
    auto matcher = cv::cuda::createStereoBM();
    auto filter = cv::cuda::createDisparityBilateralFilter();
    auto [average, std] = test_matcher(matcher, filter, dataset);
    std::cout << "BM+filter       avg=" << std::setw(6) << average
              << "ms std=" << std::setw(6) << std << "ms\n";
  }
  {
    auto matcher = cv::cuda::createStereoBM();
    // auto filter = cv::cuda::createDisparityBilateralFilter();
    auto [average, std] = test_matcher(matcher, dataset);
    std::cout << "BM              avg=" << std::setw(6) << average
              << "ms std=" << std::setw(6) << std << "ms\n";
  }
  {
    auto matcher = cv::cuda::createStereoConstantSpaceBP();
    auto filter = cv::cuda::createDisparityBilateralFilter();
    auto [average, std] = test_matcher(matcher, filter, dataset);
    std::cout << "BP+filter       avg=" << std::setw(6) << average
              << "ms std=" << std::setw(6) << std << "ms\n";
  }
  {
    auto matcher = cv::cuda::createStereoConstantSpaceBP();
    // auto filter = cv::cuda::createDisparityBilateralFilter();
    auto [average, std] = test_matcher(matcher, dataset);
    std::cout << "BP              avg=" << std::setw(6) << average
              << "ms std=" << std::setw(6) << std << "ms\n";
  }
}

#endif
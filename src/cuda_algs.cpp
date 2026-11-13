#include "cuda_algs.h"

#include <opencv2/calib3d.hpp>
#include <opencv2/core/base.hpp>
#include <opencv2/core/cuda.hpp>
#include <opencv2/core/types.hpp>
#include <opencv2/core/utility.hpp>
#include <opencv2/cudaarithm.hpp>
#include <opencv2/cudastereo.hpp>
#include <opencv2/cudawarping.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/stereo.hpp>

#include "config.h"

void cuda_test() {
  cv::cuda::Stream stream;

  cv::FileStorage file("calibration.ext", cv::FileStorage::READ);
  cv::Mat K1, K2, D1, D2, R, T;
  cv::Mat P1, P2, R1, R2, Q;
  cv::Mat disp_cpu, merged_frames;
  cv::Mat map1x, map1y, map2x, map2y;
  cv::cuda::GpuMat map1x_g, map1y_g, map2x_g, map2y_g;
  cv::cuda::GpuMat frameL, frameL_r, frameR, frameR_r, disp, disp_normalized,
      disp_filtered;

  file["K1"] >> K1;
  file["K2"] >> K2;
  file["D1"] >> D1;
  file["D2"] >> D2;
  file["R"] >> R;
  file["T"] >> T;

  cv::stereoRectify(K1, D1, K2, D2, cv::Size(CAM_WIDTH, CAM_HEIGHT), R, T, R1,
                    R2, P1, P2, Q, cv::STEREO_ZERO_DISPARITY, 0);

  cv::initUndistortRectifyMap(K1, D1, R1, P1, cv::Size(CAM_WIDTH, CAM_HEIGHT),
                              CV_32FC1, map1x, map1y);
  cv::initUndistortRectifyMap(K2, D2, R2, P2, cv::Size(CAM_WIDTH, CAM_HEIGHT),
                              CV_32FC1, map2x, map2y);

  map1x_g.upload(map1x, stream);
  map1y_g.upload(map1y, stream);
  map2x_g.upload(map2x, stream);
  map2y_g.upload(map2y, stream);

  merged_frames = cv::imread("SSD/converted/frame140.png", cv::IMREAD_GRAYSCALE);
  auto left = merged_frames.colRange(0, 1600);
  auto right = merged_frames.colRange(1600, 3200);

  frameL.upload(left);
  frameR.upload(right);

  auto matcher = cv::cuda::createStereoSGM(0, 256);
  auto filter = cv::cuda::createDisparityBilateralFilter(256, 3, 1);

  stream.waitForCompletion();

  for (int i = 0; i < 10; i++) {
    cv::cuda::Stream stream;

    cv::cuda::remap(frameL, frameL_r, map1x_g, map1y_g, cv::INTER_NEAREST,
                    cv::BORDER_CONSTANT, cv::Scalar(), stream);
    cv::cuda::remap(frameR, frameR_r, map2x_g, map2y_g, cv::INTER_NEAREST,
                    cv::BORDER_CONSTANT, cv::Scalar(), stream);

    matcher->compute(frameL_r, frameR_r, disp, stream);
    // filter->apply(disp, frameL_r, disp_filtered, stream);
    disp_filtered = std::move(disp);

    cv::cuda::normalize(disp_filtered, disp_normalized, 0, 255, cv::NORM_MINMAX,
                        CV_8UC1, cv::noArray(), stream);
    stream.waitForCompletion();
  }

  disp_normalized.download(disp_cpu);
  cv::imwrite("test_disp.png", disp_cpu);
}

#include "camera_rectify.h"

#include <opencv2/calib3d.hpp>
#include <opencv2/core/mat.hpp>
#include <opencv2/core/persistence.hpp>
#include <opencv2/imgproc.hpp>

#include "config.h"

CameraRectify::CameraRectify(Slot<std::pair<size_t, cv::Mat>> &input,
                             cv::Mat map1, cv::Mat map2)
    : PipelineElement(input), map1(map1), map2(map2) {}

std::pair<size_t, cv::Mat>
CameraRectify::work(std::pair<size_t, cv::Mat> &&input) {
  cv::Mat ret;
  auto [timestamp, frame] = input;
  cv::remap(frame, ret, map1, map2, cv::INTER_NEAREST);
  return {timestamp, ret};
}

std::pair<std::unique_ptr<CameraRectify>, std::unique_ptr<CameraRectify>>
CameraRectify::setup_rectifiers(Slot<std::pair<size_t, cv::Mat>> &cam1_input,
                                Slot<std::pair<size_t, cv::Mat>> &cam2_input,
                                std::string path_to_params) {
  cv::FileStorage file(path_to_params, cv::FileStorage::READ);
  cv::Mat K1, K2, D1, D2, R, T;
  cv::Mat P1, P2, R1, R2, Q;
  cv::Mat map1x, map1y, map2x, map2y;

  file["K1"] >> K1;
  file["K2"] >> K2;
  file["D1"] >> D1;
  file["D2"] >> D2;
  file["R"] >> R;
  file["T"] >> T;

  cv::stereoRectify(K1, D1, K2, D2, cv::Size(CAM_WIDTH, CAM_HEIGHT), R, T, R1,
                    R2, P1, P2, Q);

  cv::initUndistortRectifyMap(K1, D1, R1, P1, cv::Size(CAM_WIDTH, CAM_HEIGHT),
                              CV_32FC1, map1x, map1y);
  cv::initUndistortRectifyMap(K2, D2, R2, P2, cv::Size(CAM_WIDTH, CAM_HEIGHT),
                              CV_32FC1, map2x, map2y);

  auto rec1 = std::make_unique<CameraRectify>(cam1_input, map1x, map1y);
  auto rec2 = std::make_unique<CameraRectify>(cam2_input, map2x, map2y);
  return std::make_pair(std::move(rec1), std::move(rec2));
}

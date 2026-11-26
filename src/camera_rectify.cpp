#include "camera_rectify.h"

CameraRectify::Output CameraRectify::work(CameraRectify::Input &&frames) {
  auto [left, right] = frames;
  cv::Mat left_out, right_out;
  cv::remap(left, left_out, map_x_left, map_y_left, cv::INTER_NEAREST);
  cv::remap(right, right_out, map_x_right, map_y_right, cv::INTER_NEAREST);
  return {left_out, right_out};
}

CameraRectify::CameraRectify(Slot<PipelineElement::Input> input,
                             std::filesystem::path calibration_file)
    : PipelineElement(std::move(input)) {
  cv::FileStorage file(calibration_file, cv::FileStorage::READ);
  cv::Mat K1, K2, D1, D2, R, T;
  cv::Mat P1, P2, R1, R2, Q;

  file["K1"] >> K1;
  file["K2"] >> K2;
  file["D1"] >> D1;
  file["D2"] >> D2;
  file["R"] >> R;
  file["T"] >> T;

  cv::stereoRectify(K1, D1, K2, D2, cv::Size(CAM_WIDTH, CAM_HEIGHT), R, T, R1,
                    R2, P1, P2, Q);

  cv::initUndistortRectifyMap(K1, D1, R1, P1, cv::Size(CAM_WIDTH, CAM_HEIGHT),
                              CV_32FC1, map_x_left, map_y_left);
  cv::initUndistortRectifyMap(K2, D2, R2, P2, cv::Size(CAM_WIDTH, CAM_HEIGHT),
                              CV_32FC1, map_x_right, map_y_right);
}
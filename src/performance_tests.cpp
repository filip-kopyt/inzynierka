#include "performance_tests.h"

#include <array>
#include <cstddef>
#include <filesystem>
#include <fstream>
#include <ios>
#include <iostream>
#include <opencv2/core/cvstd_wrapper.hpp>
#include <opencv2/core/mat.hpp>
#include <opencv2/cudastereo.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/stereo.hpp>
#include <opencv4/opencv2/core/base.hpp>
#include <string>
#include <utility>
#include <vector>

#include "config.h"

struct DataRange {
  int series_id;
  int start_frame_id;
  int end_frame_id;
};

struct Remaps {
  cv::Mat map_lx, map_ly;
  cv::Mat map_rx, map_ry;
};

const std::filesystem::path DATA_FOLDER = "/home/filip-kopyt/jetson-data";

const size_t RUNS = 25;
const std::array DATA_SERIES = {
    DataRange{.series_id = 0, .start_frame_id = 50, .end_frame_id = 100},
};

static inline Remaps load_remaps() {
  cv::FileStorage file("calibration.ext", cv::FileStorage::READ);
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
                    R2, P1, P2, Q, cv::STEREO_ZERO_DISPARITY, 0);

  cv::initUndistortRectifyMap(K1, D1, R1, P1, cv::Size(CAM_WIDTH, CAM_HEIGHT),
                              CV_32FC1, map1x, map1y);
  cv::initUndistortRectifyMap(K2, D2, R2, P2, cv::Size(CAM_WIDTH, CAM_HEIGHT),
                              CV_32FC1, map2x, map2y);

  return Remaps{
      .map_lx = map1x, .map_ly = map1y, .map_rx = map2x, .map_ry = map2y};
}

std::vector<std::pair<cv::Mat, cv::Mat>> load_dataset() {
  std::vector<std::pair<cv::Mat, cv::Mat>> ret;
  const auto remaps = load_remaps();
  std::cout << "dataset loading\n";

  for (const auto &series : DATA_SERIES) {
    for (int frame_id = series.start_frame_id; frame_id < series.end_frame_id;
         frame_id++) {
      auto filepath = DATA_FOLDER /
                      ("frames" + std::to_string(series.series_id)) /
                      ("frame" + std::to_string(frame_id) + ".raw");
      std::ifstream file{filepath, std::ios_base::binary | std::ios_base::in};
      auto databuf = std::vector(std::istreambuf_iterator{file}, {});
      cv::Mat frames_merged =
          cv::Mat(CAM_HEIGHT, CAM_WIDTH * 2, CV_8UC1, databuf.data()).clone();
      auto left = frames_merged.colRange(0, CAM_WIDTH);
      auto right = frames_merged.colRange(CAM_WIDTH, 2 * CAM_WIDTH);

      cv::Mat left_remapped, right_remapped;
      cv::remap(left, left_remapped, remaps.map_lx, remaps.map_ly,
                cv::INTER_LINEAR);
      cv::remap(right, right_remapped, remaps.map_rx, remaps.map_ry,
                cv::INTER_LINEAR);

      ret.push_back({left_remapped, right_remapped});
    }
  }
  std::cout << "dataset loaded\n";
  return ret;
}
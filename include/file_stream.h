#pragma once

#include <chrono>
#include <filesystem>
#include <opencv2/core/mat.hpp>
#include <utility>
#include <vector>

#include "pipeline_element.hpp"

class FileStream : public PipelineElement<void, std::pair<cv::Mat, cv::Mat>> {
private:
  std::vector<std::pair<cv::Mat, cv::Mat>> _frames;
  size_t _frames_idx = 0;
  size_t _framerate = 24;
  std::chrono::time_point<std::chrono::high_resolution_clock> _last_frame =
      std::chrono::high_resolution_clock::now();

public:
  FileStream(std::filesystem::path frames_dir);

  virtual PipelineElement::Output work() override;
};
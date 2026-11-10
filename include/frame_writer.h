#pragma once

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <fcntl.h>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <memory>
#include <mutex>
#include <numeric>
#include <opencv2/core.hpp>
#include <opencv2/imgcodecs.hpp>
#include <queue>
#include <ratio>
#include <sstream>
#include <unistd.h>
#include <utility>

#include "config.h"
#include "pipeline_element.hpp"

class FrameWriter : public PipelineElement<std::pair<cv::Mat, cv::Mat>, void> {
private:
  std::atomic<int> _counter = 0;
  size_t _max_size = 100;
  size_t _workers_no = 4;

  std::filesystem::path _save_loc;
  std::mutex _mutex;
  std::condition_variable _cv;
  std::queue<std::pair<cv::Mat, cv::Mat>> _queue;
  std::vector<std::thread> _workers;

  // std::mutex _times_mutex;
  // std::vector<double> _encoding_times_msec;
  // std::vector<double> _writing_times_msec;

public:
  FrameWriter(std::filesystem::path save_loc, Slot<std::pair<cv::Mat, cv::Mat>> &input);

  virtual void work(std::pair<cv::Mat, cv::Mat> &&in) override;

  std::pair<cv::Mat, cv::Mat> get_frames();

  void worker_thread();

  virtual std::thread start() override;

  virtual void stop() override;
};

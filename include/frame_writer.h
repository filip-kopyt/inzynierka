#pragma once

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <cstdlib>
#include <fcntl.h>
#include <fstream>
#include <iostream>
#include <memory>
#include <mutex>
#include <opencv2/core.hpp>
#include <opencv2/imgcodecs.hpp>
#include <queue>
#include <ratio>
#include <unistd.h>
#include <utility>
#include <numeric>
#include <filesystem>

#include "config.h"
#include "pipeline_element.hpp"

class FrameWriter : public PipelineElement<std::pair<cv::Mat, cv::Mat>, void> {
private:
  std::atomic<int> _counter = 0;
  int _max_size = 100;
  int _workers_no = 4;

  std::filesystem::path _save_loc;
  std::mutex _mutex;
  std::condition_variable _cv;
  std::queue<std::pair<cv::Mat, cv::Mat>> _queue;
  std::vector<std::thread> _workers;

  std::mutex _times_mutex;
  std::vector<double> _encoding_times_msec;
  std::vector<double> _writing_times_msec;

public:
  FrameWriter(std::filesystem::path save_loc, auto &input) : PipelineElement(input), _save_loc(save_loc) {}

  virtual void work(std::pair<cv::Mat, cv::Mat> &&in) override {
    std::unique_lock lock{_mutex};
    if (_queue.size() > _max_size) {
      //std::cout << "throttling";
      if (!_cv.wait_for(lock, std::chrono::seconds(1), [&]() { return _queue.size() <= _max_size; })) {
        return;
      }
    }
    _queue.push(in);
    _cv.notify_all();
  }

  std::pair<cv::Mat, cv::Mat> get_frames() {
    std::unique_lock lock{_mutex};
    if (_queue.empty()) {
      _cv.wait(lock, [&]() { return !_queue.empty(); });
    }
    auto item = _queue.front();
    _queue.pop();
    _cv.notify_all();
    return item;
  }

  void worker_thread() {
    while (this->is_running()) {
      auto [frame1, frame2] = get_frames();
      if (frame1.empty() || frame2.empty())
        return;

      cv::Mat merged_frames;
      auto file_name = _save_loc / ("frame" + std::to_string(_counter) + ".png");
      _counter++;

      cv::hconcat(std::array{frame1, frame2}, merged_frames);

      std::vector<uint8_t> png_buf;

      auto start = std::chrono::high_resolution_clock::now();
      cv::imencode(".png", merged_frames, png_buf, {cv::IMWRITE_PNG_COMPRESSION, 1});
      std::chrono::duration<double, std::milli> encoding_duration = std::chrono::high_resolution_clock::now() - start;

      auto file = open(file_name.c_str(), O_WRONLY | O_SYNC | O_CREAT | O_DIRECT);

      start = std::chrono::high_resolution_clock::now();
      write(file, png_buf.data(), png_buf.size());
      std::chrono::duration<double, std::milli> writing_duration = std::chrono::high_resolution_clock::now() - start;

      std::unique_lock lock{_times_mutex};
      _encoding_times_msec.push_back(encoding_duration.count());
      _writing_times_msec.push_back(writing_duration.count());

      if (_counter >= MAX_FRAMES && this->is_running()) {
        std::cout << "max frames written\n";
        std::cout << "encoding time avg: " << std::accumulate(_encoding_times_msec.begin(), _encoding_times_msec.end(), 0) / (double)_encoding_times_msec.size() << "ms\n";
        std::cout << "writing time avg: " << std::accumulate(_writing_times_msec.begin(), _writing_times_msec.end(), 0) / (double)_writing_times_msec.size() << "ms\n";
        this->stop();
      }
    }
  }

  std::thread start() override {
    for (int i = 0; i < _workers_no; i++) {
        std::thread(&FrameWriter::worker_thread, this).detach();
    }
    return std::thread(&FrameWriter::loop, this);
  }
};
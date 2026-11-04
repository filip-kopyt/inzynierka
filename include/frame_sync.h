#pragma once

#include <iostream>
#include <memory>
#include <opencv2/core.hpp>
#include <thread>

#include "config.h"
#include "pipeline_element.hpp"

class FrameSync : public PipelineElementBase {
private:
  Slot<std::pair<uint64_t, cv::Mat>> &_in1;
  Slot<std::pair<uint64_t, cv::Mat>> &_in2;
  Slot<std::pair<cv::Mat, cv::Mat>> _out;

public:
  FrameSync(auto &in1, auto &in2) : _in1(in1), _in2(in2) {}

  void loop() {
    while (is_running()) {
      auto opt1 = _in1.pull();
      while (!opt1 && is_running()) opt1 = _in1.pull();

      auto opt2 = _in2.pull();
      while (!opt2 && is_running()) opt2 = _in2.pull();

      if (!is_running()) return;

      auto [timestamp1, frame1] = *opt1;
      auto [timestamp2, frame2] = *opt2;

      while (std::abs((int64_t)timestamp1 - (int64_t)timestamp2) >=
             MAX_FRAME_TIMESTAMP_DIFF_NS && is_running()) {
        if (timestamp1 > timestamp2) {
          auto opt = _in2.pull();
          while (!opt && is_running()) opt = _in2.pull();
          if (!is_running()) return;
          std::tie(timestamp2, frame2) = *opt;
        } else {
          auto opt = _in1.pull();
          while (!opt && is_running()) opt = _in1.pull();
          if (!is_running()) return;
          std::tie(timestamp1, frame1) = *opt;
        }
        std::cout << "dropped frames not in sync\n";
      }

      _out.push({frame1, frame2});
    }
  }

  Slot<std::pair<cv::Mat, cv::Mat>> &output() { return _out; }

  virtual std::thread start() override { return std::thread(&FrameSync::loop, this); }
};
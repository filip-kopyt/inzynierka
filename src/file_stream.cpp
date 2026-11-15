#include "file_stream.h"

#include <chrono>
#include <fstream>
#include <ranges>
#include <regex>
#include <thread>

#include "config.h"

FileStream::FileStream(std::filesystem::path frames_dir) : PipelineElement() {
  std::vector<std::filesystem::path> paths;

  for (const auto &file : std::filesystem::directory_iterator(frames_dir)) {
    assert(file.is_regular_file());
    paths.push_back(file);
  }

  auto number_finder = std::regex("frame([0-9]*)\\.raw");

  auto frame_number = [&](const std::filesystem::path &p) -> unsigned int {
    std::smatch sm;
    const std::string filename = p.filename().string();
    assert(std::regex_match(filename, sm, number_finder));
    return std::stoul(sm[1].str());
  };

  std::sort(paths.begin(), paths.end(),
            [&](const std::filesystem::path &a, const std::filesystem::path &b)
                -> bool { return frame_number(a) < frame_number(b); });

  for (const auto &path : paths | std::views::take(240)) {
    auto file = std::ifstream(path, std::ios::binary);
    auto databuf = std::vector(std::istreambuf_iterator{file}, {});
    auto frame =
        cv::Mat(CAM_HEIGHT, CAM_WIDTH * 2, CV_8UC1, databuf.data()).clone();
    auto left = frame.colRange(0, CAM_WIDTH);
    auto right = frame.colRange(CAM_WIDTH, 2 * CAM_WIDTH);

    _frames.push_back({left, right});
  }
}

FileStream::PipelineElement::Output FileStream::work() {
  _frames_idx %= _frames.size();

  std::this_thread::sleep_until(_last_frame +
                                std::chrono::milliseconds(1000 / _framerate));
  _last_frame = std::chrono::high_resolution_clock::now();

  return _frames[_frames_idx++];
}

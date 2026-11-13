#include "frame_writer.h"

FrameWriter::FrameWriter(std::filesystem::path save_loc,
                         Slot<std::pair<cv::Mat, cv::Mat>> &input)
    : PipelineElement(input) {
  std::stringstream ss;
  auto t = std::time(nullptr);
  auto tm = *std::localtime(&t);
  ss << std::put_time(&tm, "%y-%m-%d-%H-%M-%S");

  _save_loc = save_loc;
  std::filesystem::create_directories(_save_loc);
}

void FrameWriter::work(std::pair<cv::Mat, cv::Mat> &&in) {
  std::unique_lock lock{_mutex};
  if (_queue.size() > _max_size) {
    // std::cout << "throttling";
    if (!_cv.wait_for(lock, std::chrono::seconds(1),
                      [&]() { return _queue.size() <= _max_size; })) {
      return;
    }
  }
  _queue.push(in);
  _cv.notify_all();
}

std::pair<cv::Mat, cv::Mat> FrameWriter::get_frames() {
  std::unique_lock lock{_mutex};
  if (_queue.empty()) {
    if (!_cv.wait_for(lock, std::chrono::seconds(1),
                      [&]() { return !_queue.empty(); })) {
      return {cv::Mat(), cv::Mat()};
    }
  }
  auto item = _queue.front();
  _queue.pop();
  _cv.notify_all();
  return item;
}

void FrameWriter::worker_thread() {
  while (this->is_running()) {
    auto [frame1, frame2] = get_frames();
    if (frame1.empty() || frame2.empty())
      return;

    cv::Mat merged_frames;
    auto file_name = _save_loc / ("frame" + std::to_string(_counter) + ".raw");
    _counter++;

    cv::hconcat(std::array{frame1, frame2}, merged_frames);
    assert(merged_frames.isContinuous());

    auto file = std::ofstream(file_name, std::ios::binary);
    file.write(reinterpret_cast<char *>(merged_frames.data),
               merged_frames.total() * merged_frames.elemSize());
    file.close();

    // if (_counter >= 50) {
    //   exit(0);
    // }
  }
}

std::thread FrameWriter::start() {
  for (size_t i = 0; i < _workers_no; i++) {
    _workers.push_back(std::thread(&FrameWriter::worker_thread, this));
  }
  return std::thread(&FrameWriter::loop, this);
}

void FrameWriter::stop() {
  PipelineElement::stop();
  for (auto &thread : _workers) {
    thread.join();
  }
}
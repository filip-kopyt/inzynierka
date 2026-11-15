#include <opencv2/stereo.hpp>
#include <signal.h>

#include "camera_rectify.h"
#include "cuda_algs.h"
#include "disparity_calculation.h"
#include "exposure_regulator.h"
#include "file_stream.h"
#include "frame_writer.h"
#include "stereo_camera.h"
#include "video_display.h"

static bool running = true;

void signal_handler(int) {
  std::cout << "stopping\n";
  running = false;
}

void run(std::vector<PipelineElementBase *> parts) {
  std::vector<std::thread> threads;
  signal(SIGINT, &signal_handler);

  std::cout << "Starting\n";

  for (const auto &part : parts) {
    threads.push_back(part->start());
  }

  while (running)
    std::this_thread::yield();

  for (const auto &part : parts) {
    part->stop();
  }

  for (auto &thread : threads) {
    thread.join();
  }
}

int main() {
  auto matcher = cv::StereoSGBM::create(0, 128, 1, 10, 40, 2, 16, 15, 50, 1);

  auto file_stream = FileStream("/home/filip-kopyt/jetson-data/frames6");
  auto rectify = CameraRectify(file_stream.output(), "calibration.ext");
  auto disparity = DisparityCalculation(matcher, rectify.output());
  auto display = VideoDisplay(disparity.output());

  run({&file_stream, &rectify, &disparity, &display});
}

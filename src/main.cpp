#include <signal.h>

#include "camera_rectify.h"
#include "exposure_regulator.h"
#include "stereo_camera.h"
#include "frame_writer.h"
#include "video_display.h"

static bool running = true;

void signal_handler(int signal) {
  std::cout << "stopping\n";
  running = false;
}

void run(std::vector<PipelineElementBase *> parts) {
  std::vector<std::thread> threads;
  signal(SIGINT, &signal_handler);

  for (const auto v : parts) {
    threads.push_back(v->start());
  }

  while (running)
    std::this_thread::yield();

  for (const auto v : parts) {
    v->stop();
  }

  for (auto &t : threads) {
    t.join();
  }
}

int main(int argc, char *argv[]) {
  // cuda_test();
  // return 0;

  if (argc != 3) {
    std::cout << "usage: " << argv[0]
              << " <calibration_file>  <frame_destination>\n";
    return 1;
  }

  auto cams = StereoCamera();

  auto tee = Tee(cams.output());
  // auto tee2 = Tee(tee.out1());

  // auto viewer = VideoDisplay(tee2.out2());
  auto consumer = FrameWriter(argv[2], tee.out1());
  auto exposure_regulator =
      ExposureRegulator(tee.out2(), cams, 0.15, 0.05, 0.20);

  run({
      &cams,
      &tee,
      //&tee2,
      &exposure_regulator,
      //&viewer,
      &consumer,
  });
}

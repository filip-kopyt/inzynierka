#include "camera_rectify.h"
#include "exposure_regulator.h"
#include "stereo_camera.h"
//#include "frame_sync.h"
#include "frame_writer.h"
#include "video_display.h"
#include <opencv2/core/base.hpp>
#include <opencv2/core/cuda.hpp>
#include <opencv2/core/hal/interface.h>
#include <opencv2/core/types.hpp>
#include <opencv2/core/utility.hpp>
//#include <opencv2/cudaarithm.hpp>
//#include <opencv2/cudastereo.hpp>
//#include <opencv2/cudawarping.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>
//#include <opencv2/stereo.hpp>
#include <signal.h>

// void cuda_test() {
//   cv::cuda::Stream stream;

//   cv::FileStorage file("calibration.ext", cv::FileStorage::READ);
//   cv::Mat K1, K2, D1, D2, R, T;
//   cv::Mat P1, P2, R1, R2, Q;
//   cv::Mat disp_cpu;
//   cv::Mat map1x, map1y, map2x, map2y;
//   cv::cuda::GpuMat map1x_g, map1y_g, map2x_g, map2y_g;
//   cv::cuda::GpuMat frameL, frameL_r, frameR, frameR_r, disp, disp_normalized,
//       disp_filtered;

//   file["K1"] >> K1;
//   file["K2"] >> K2;
//   file["D1"] >> D1;
//   file["D2"] >> D2;
//   file["R"] >> R;
//   file["T"] >> T;

//   cv::stereoRectify(K1, D1, K2, D2, cv::Size(CAM_WIDTH, CAM_HEIGHT), R, T,
//   R1,
//                     R2, P1, P2, Q, cv::STEREO_ZERO_DISPARITY, 0);

//   cv::initUndistortRectifyMap(K1, D1, R1, P1, cv::Size(CAM_WIDTH,
//   CAM_HEIGHT),
//                               CV_32FC1, map1x, map1y);
//   cv::initUndistortRectifyMap(K2, D2, R2, P2, cv::Size(CAM_WIDTH,
//   CAM_HEIGHT),
//                               CV_32FC1, map2x, map2y);

//   map1x_g.upload(map1x, stream);
//   map1y_g.upload(map1y, stream);
//   map2x_g.upload(map2x, stream);
//   map2y_g.upload(map2y, stream);
//   frameL.upload(cv::imread("../left/frame0.png", cv::IMREAD_GRAYSCALE),
//   stream); frameR.upload(cv::imread("../right/frame0.png",
//   cv::IMREAD_GRAYSCALE),
//                 stream);

//   auto matcher = cv::cuda::createStereoConstantSpaceBP(256);
//   auto filter = cv::cuda::createDisparityBilateralFilter(256, 3, 1);

//   stream.waitForCompletion();

//   for (int i = 0; i < 100; i++) {
//     cv::cuda::Stream stream;

//     cv::cuda::remap(frameL, frameL_r, map1x_g, map1y_g, cv::INTER_NEAREST,
//                     cv::BORDER_CONSTANT, cv::Scalar(), stream);
//     cv::cuda::remap(frameR, frameR_r, map2x_g, map2y_g, cv::INTER_NEAREST,
//                     cv::BORDER_CONSTANT, cv::Scalar(), stream);

//     matcher->compute(frameL_r, frameR_r, disp, stream);
//     // filter->apply(disp, frameL_r, disp_filtered, stream);
//     disp_filtered = std::move(disp);

//     cv::cuda::normalize(disp_filtered, disp_normalized, 0, 255,
//     cv::NORM_MINMAX,
//                         CV_8UC1, cv::noArray(), stream);
//     stream.waitForCompletion();
//   }

//   disp_normalized.download(disp_cpu);
//   cv::imwrite("test_disp.png", disp_cpu);
// }

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
  // auto frame_sync = FrameSync(cams.output());
  // auto [rec1, rec2] =
  //    CameraRectify::setup_rectifiers(cam1.output(), cam2.output(), argv[1]);

  auto tee = Tee(cams.output());
  //auto tee2 = Tee(tee.out1());

  //auto viewer = VideoDisplay(tee2.out2());
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

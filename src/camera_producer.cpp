#include "camera_producer.h"

#include <cstring>
#include <fcntl.h>
#include <ios>
#include <linux/v4l2-controls.h>
#include <linux/videodev2.h>
#include <opencv2/core/cuda.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/opencv.hpp>
#include <sstream>
#include <string>
#include <sys/ioctl.h>
#include <sys/mman.h>

#include "config.h"

static inline int get_camera_fd(int cam_index) {
  int fd;
  std::string path = "/dev/video" + std::to_string(cam_index);
  if ((fd = open(path.c_str(), O_RDWR)) < 0) {
    auto msg = "open " + path;
    perror(msg.c_str());
    exit(1);
  }
  return fd;
}

static inline void set_cam_ctrl(int fd, unsigned int id, int value) {
  v4l2_control ctrl;
  ctrl.id = id;
  ctrl.value = value;

  if (ioctl(fd, VIDIOC_S_CTRL, &ctrl) < 0) {
    std::stringstream msg;
    msg << "failed to set ctrl id=" << std::hex << id;
    perror(msg.str().c_str());
    exit(1);
  }
}

static inline void define_format(int fd) {
  v4l2_format fmt;
  memset(&fmt, 0, sizeof(fmt));
  fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
  fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_GREY;
  fmt.fmt.pix.width = CAM_WIDTH;
  fmt.fmt.pix.height = CAM_HEIGHT;

  if (ioctl(fd, VIDIOC_S_FMT, &fmt) < 0) {
    perror("VIDIOC_S_FMT");
    exit(1);
  }
}

static inline void request_buffer(int fd) {
  v4l2_requestbuffers req;
  memset(&req, 0, sizeof(req));
  req.count = 1;
  req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
  req.memory = V4L2_MEMORY_MMAP;

  if (ioctl(fd, VIDIOC_REQBUFS, &req) < 0) {
    perror("VIDIOC_REQBUFS");
    exit(1);
  }
}

static inline void *querry_buffer(int fd) {
  v4l2_buffer buf;
  memset(&buf, 0, sizeof(buf));
  buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
  buf.memory = V4L2_MEMORY_MMAP;
  buf.index = 0;

  if (ioctl(fd, VIDIOC_QUERYBUF, &buf) < 0) {
    perror("VIDIOC_QUERYBUF");
    exit(1);
  }

  auto buffer_ptr = mmap(NULL, CAM_BUF_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED,
                         fd, buf.m.offset);
  if (buffer_ptr == MAP_FAILED) {
    perror("mmap");
  }

  return buffer_ptr;
}

static inline void enable_streaming(int fd) {
  v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
  if (ioctl(fd, VIDIOC_STREAMON, &type) < 0) {
    perror("VIDIOC_STREAMON");
    exit(1);
  }
}

static inline int dequeue_buffer(int fd) {
  v4l2_buffer buf;
  memset(&buf, 0, sizeof(buf));
  buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
  buf.memory = V4L2_MEMORY_MMAP;

  if (ioctl(fd, VIDIOC_DQBUF, &buf) < 0) {
    perror("DQBUF");
    exit(1);
  }
  uint64_t timestamp =
      ((uint64_t)buf.timestamp.tv_sec * 1000000 + buf.timestamp.tv_usec) * 1000;

  return timestamp;
}

static inline void enqueue_buffer(int fd) {
  v4l2_buffer buf;

  memset(&buf, 0, sizeof(buf));
  buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
  buf.memory = V4L2_MEMORY_MMAP;
  buf.index = 0;

  if (ioctl(fd, VIDIOC_QBUF, &buf) < 0) {
    perror("VIDIOC_QBUF");
    exit(1);
  }
}

CameraProducer::CameraProducer(int camera_index) {
  _cam_fd = get_camera_fd(camera_index);
  define_format(_cam_fd);
  request_buffer(_cam_fd);
  _cam_buf = querry_buffer(_cam_fd);
  enqueue_buffer(_cam_fd);
  enable_streaming(_cam_fd);
  set_cam_ctrl(_cam_fd, CTRL_ID_FRAMERATE, CAM_FPS);
}

std::pair<size_t, cv::Mat> CameraProducer::work() {
  auto timestamp = dequeue_buffer(_cam_fd);

  auto frame =
      cv::Mat(cv::Size(CAM_WIDTH, CAM_HEIGHT), CV_8UC1, _cam_buf);
  _counter++;
#if 0
  cv::cuda::GpuMat gpu_frame;
  gpu_frame.upload(frame);
#endif

  enqueue_buffer(_cam_fd);
  return {timestamp, frame};
}

void CameraProducer::update_exposure(int exposure) {
  // OV2311s need ~100ms to update framerate, 
  // setting exposure in that timeframe resets framerate
  if (_counter < 5)
    return;
  set_cam_ctrl(_cam_fd, CTRL_ID_EXPOSURE, exposure);
}

CameraProducer::~CameraProducer() {
  munmap(_cam_buf, CAM_BUF_SIZE);
  close(_cam_fd);
}

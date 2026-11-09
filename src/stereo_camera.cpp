#include "stereo_camera.h"

#include <cerrno>
#include <cstring>
#include <fcntl.h>
#include <gpiod.hpp>
#include <ios>
#include <linux/v4l2-controls.h>
#include <linux/videodev2.h>
#include <opencv2/core/cuda.hpp>
#include <opencv2/core/mat.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/opencv.hpp>
#include <sstream>
#include <stdexcept>
#include <string>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <thread>
#include <tuple>

#include "config.h"

#define THROW(msg)                                                             \
  throw std::runtime_error(std::string(msg) + ": " + strerror(errno))

static inline FileDescriptor get_camera_fd(int cam_index) {
  int fd;
  std::string path = "/dev/video" + std::to_string(cam_index);
  if ((fd = open(path.c_str(), O_RDWR)) < 0) {
    THROW("open " + path);
  }
  return fd;
}

static inline void set_cam_ctrl(FileDescriptor fd, unsigned int id, int value) {
  v4l2_control ctrl;
  ctrl.id = id;
  ctrl.value = value;

  if (ioctl(fd, VIDIOC_S_CTRL, &ctrl) < 0) {
    std::stringstream msg;
    msg << "failed to set ctrl id=" << std::hex << id;
    THROW(msg.str());
  }
}

static inline void define_format(FileDescriptor fd) {
  v4l2_format fmt;
  memset(&fmt, 0, sizeof(fmt));
  fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
  fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_GREY;
  fmt.fmt.pix.width = CAM_WIDTH;
  fmt.fmt.pix.height = CAM_HEIGHT;

  if (ioctl(fd, VIDIOC_S_FMT, &fmt) < 0) {
    THROW("VIDIOC_S_FMT");
  }
}

static inline void request_buffer(FileDescriptor fd) {
  v4l2_requestbuffers req;
  memset(&req, 0, sizeof(req));
  req.count = 1;
  req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
  req.memory = V4L2_MEMORY_MMAP;

  if (ioctl(fd, VIDIOC_REQBUFS, &req) < 0) {
    THROW("VIDIOC_REQBUFS");
  }
}

static inline BufferPtr querry_buffer(FileDescriptor fd) {
  v4l2_buffer buf;
  memset(&buf, 0, sizeof(buf));
  buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
  buf.memory = V4L2_MEMORY_MMAP;
  buf.index = 0;

  if (ioctl(fd, VIDIOC_QUERYBUF, &buf) < 0) {
    THROW("VIDIOC_QUERYBUF");
  }

  auto buffer_ptr = mmap(NULL, CAM_BUF_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED,
                         fd, buf.m.offset);
  if (buffer_ptr == MAP_FAILED) {
    THROW("mmap");
  }

  return buffer_ptr;
}

static inline void enable_streaming(FileDescriptor fd) {
  v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
  if (ioctl(fd, VIDIOC_STREAMON, &type) < 0) {
    THROW("VIDIOC_STREAMON");
  }
}

static inline uint64_t dequeue_buffer(FileDescriptor fd) {
  v4l2_buffer buf;
  memset(&buf, 0, sizeof(buf));
  buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
  buf.memory = V4L2_MEMORY_MMAP;

  if (ioctl(fd, VIDIOC_DQBUF, &buf) < 0) {
    THROW("VIDIOC_DQBUF");
  }
  auto tv = buf.timestamp;
  return (uint64_t)tv.tv_sec * 1000000 + tv.tv_usec;
}

static inline void enqueue_buffer(FileDescriptor fd) {
  v4l2_buffer buf;

  memset(&buf, 0, sizeof(buf));
  buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
  buf.memory = V4L2_MEMORY_MMAP;
  buf.index = 0;

  if (ioctl(fd, VIDIOC_QBUF, &buf) < 0) {
    THROW("VIDIOC_QBUF");
  }
}

static inline std::tuple<FileDescriptor, BufferPtr>
init_camera(int camera_index) {
  auto fd = get_camera_fd(camera_index);
  define_format(fd);
  request_buffer(fd);
  auto buf_ptr = querry_buffer(fd);
  enqueue_buffer(fd);
  enable_streaming(fd);
  set_cam_ctrl(fd, CTRL_ID_TRIGGER_MODE, 1);
  //set_cam_ctrl(fd, CTRL_ID_DISABLE_FRAME_TIMEOUT, 1);
  return {fd, buf_ptr};
}

static inline gpiod::line setup_gpio(gpiod::chip &chip) {
  auto line = chip.get_line(GPIO_PWM_LINE_OFFSET);
  gpiod::line_request config;
  config.consumer = "StereoDisparityReader";
  config.request_type = gpiod::line_request::DIRECTION_OUTPUT;
  line.request(config, 0);
  return line;
}

void StereoCamera::enable_pwm() { _gpio_line.set_value(1); }

void StereoCamera::disable_pwm() { _gpio_line.set_value(0); }

void StereoCamera::stop() {
  left.running = false;
  right.running = false;
  PipelineElementBase::stop();

  left.collection_thread.join();
  right.collection_thread.join();
}

void StereoCamera::CameraMeta::loop() {
  while (running) {
    auto timestamp = dequeue_buffer(fd);
    auto frame =
        cv::Mat(cv::Size(CAM_WIDTH, CAM_HEIGHT), CV_8UC1, buffer).clone();
    enqueue_buffer(fd);
    sink.push(frame);
  }
}

void StereoCamera::update_exposure(int value) {
  set_cam_ctrl(left.fd, CTRL_ID_EXPOSURE, value);
  set_cam_ctrl(right.fd, CTRL_ID_EXPOSURE, value);
}

std::pair<cv::Mat, cv::Mat> StereoCamera::work() {
  auto frame_l = *left.sink.pull();
  auto frame_r = *right.sink.pull();
  return {frame_l, frame_r};
}

static inline void flush_buffer(int fd) {
  int flags = fcntl(fd, F_GETFL);
  fcntl(fd, F_SETFL, flags | O_NONBLOCK);

  int again_count = 0;
  while (again_count < 10) {
    struct v4l2_buffer buf;

    memset(&buf, 0, sizeof(buf));
    buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    buf.memory = V4L2_MEMORY_MMAP;

    if (ioctl(fd, VIDIOC_DQBUF, &buf) < 0) {
      if (errno == EAGAIN) {
        again_count++;
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        continue;
      } else {
        THROW("VIDIOC_DQBUF");
      }
    }

    memset(&buf, 0, sizeof(buf));
    buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    buf.memory = V4L2_MEMORY_MMAP;
    buf.index = 0;

    if (ioctl(fd, VIDIOC_QBUF, &buf) < 0) {
      THROW("VIDIOC_QBUF");
    }
  }

  flags = fcntl(fd, F_GETFL);
  fcntl(fd, F_SETFL, flags & ~O_NONBLOCK);
}

StereoCamera::StereoCamera() : _gpio_line(setup_gpio(_gpio_chip)) {
  disable_pwm();

  std::tie(left.fd, left.buffer) = init_camera(0);
  std::tie(right.fd, right.buffer) = init_camera(1);

  flush_buffer(left.fd);
  flush_buffer(right.fd);

  enable_pwm();

  left.collection_thread = std::thread(&CameraMeta::loop, &left);
  right.collection_thread = std::thread(&CameraMeta::loop, &right);
}
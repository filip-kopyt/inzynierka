#pragma once

// Cameras' configurable parameters IDs
// via `v4l2-ctl -d /dev/video0 --list-ctrls
#define CTRL_ID_EXPOSURE 0x00980911
#define CTRL_ID_FRAMERATE 0x00981906
#define CTRL_ID_DISABLE_FRAME_TIMEOUT 0x00981902
#define CTRL_ID_FRAME_TIMEOUT 0x00981903
#define CTRL_ID_TRIGGER_MODE 0x00981901

#define GPIO_PWM_LINE_OFFSET 110

// Camera Parameters
// #define CAM_FPS 24
#define CAM_TARGET_BRIGHTNESS 100
#define CAM_WIDTH 1600
#define CAM_HEIGHT 1300
#define CAM_BUF_SIZE (CAM_WIDTH * CAM_HEIGHT)

// Stop criteria
// #define MAX_FRAMES 240

// Frame sync
// #define MAX_FRAME_TIMESTAMP_DIFF_USEC 10'000'000

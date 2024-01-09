#ifndef __DRM_DISPLAY_H__
#define __DRM_DISPLAY_H__

// #include "../rkrga/RGA.h"
#include <stdio.h>
#include <unistd.h>

extern "C"
{
#include <stdio.h>
#include <unistd.h>
#include <sys/time.h>

#include "vpu.h"
#include "rk_mpi.h"
#include "rk_type.h"
#include "vpu_api.h"
#include "mpp_err.h"
#include "mpp_task.h"
#include "mpp_meta.h"
#include "mpp_frame.h"
#include "mpp_buffer.h"
#include "mpp_packet.h"
#include "rk_mpi_cmd.h"

#include "bo.h"
#include "dev.h"
#include "modeset.h"
};

// #define SZ_4K 4096
#define PKT_SIZE SZ_4K
#define CODEC_ALIGN(x, a) (((x) + (a)-1) & ~((a)-1))
#define LCD_0_CRTC_ID 184
#define LCD_0_PLANE_ID 170

#define SCREEN_HEIGHT 1280
#define SCREEN_WIDTH 800

// lcd的宽度是800, 高度是1280
#define LCD_SCREEN_HEIGHT 1280
#define LCD_SCREEN_WIDTH 800

#define HDMI_SCREEN_HEIGHT 1080
#define HDMI_SCREEN_WIDTH 1920

// 1072/800 = 1.34
// 4224/3136 = 1.34, 基本符合长宽比, 因为opencv的策略就是等宽高缩放.
// 但是这个比例不是很好, 会导致显示的时候有黑边
// #define VIDEO_HEIGHT 800
// #define VIDEO_WIDTH 1072

// 1280/1.34 = 1715 1712能被16整除
#define VIDEO_HEIGHT 960
#define VIDEO_WIDTH 1280

#define CAM1_VIDEO_WIDTH 1280
#define CAM1_VIDEO_HEIGHT 960

// #define CAM2_VIDEO_HEIGHT 1080
#define CAM2_VIDEO_HEIGHT 1440
#define CAM2_VIDEO_WIDTH 1920

#define MODLE_OUTPUT_NUM 3

// #define VIDEO_HEIGHT 800
// #define VIDEO_WIDTH 1440

// #define DISPLAY_WIDTH VIDEO_HEIGHT
// #define DISPLAY_HEIGHT 1280

// 输出的测试图片高宽跟摄像头的输入分辨率高宽正好反过来
#define OUTPUT_IMG_WIDTH VIDEO_HEIGHT
#define OUTPUT_IMG_HEIGHT VIDEO_WIDTH

#define CAM1_OUTPUT_IMG_WIDTH CAM1_VIDEO_HEIGHT
#define CAM1_OUTPUT_IMG_HEIGHT CAM1_VIDEO_WIDTH

#define LCD_DATA_HEIGHT CAM1_VIDEO_WIDTH
#define LCD_DATA_WIDTH CAM1_VIDEO_HEIGHT

#define HDMI_DATA_HEIGHT CAM2_VIDEO_WIDTH
#define HDMI_DATA_WIDTH CAM2_VIDEO_HEIGHT

// 裁掉的高度部分
#define CORP_HEIGHT (VIDEO_WIDTH - SCREEN_HEIGHT)

#define OUTPUT_DEVICE_HDMI 1
#define OUTPUT_DEVICE_LCD 2

#define RGAX_SIZE
#define cxx_log(fmt, ...)           \
    do                              \
    {                               \
        printf(fmt, ##__VA_ARGS__); \
    } while (0)

void init_display();

#endif

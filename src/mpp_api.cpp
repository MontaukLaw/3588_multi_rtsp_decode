
#include <dlfcn.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/time.h>
#include <opencv2/opencv.hpp>

#include "im2d.h"
#include "rga.h"
#include "RgaUtils.h"

#include "rknn_api.h"

#include "rkmedia/utils/mpp_decoder.h"
#include "rkmedia/utils/mpp_encoder.h"

#include "mk_mediakit.h"
#include "task/yolov8_custom.h"
#include "draw/cv_draw.h"
#include "task/yolov8_thread_pool.h"
#include "mpp_api.h"
#include "screen_test.h"
#include <iostream>
#include <iomanip>
#include <chrono>
#include <ctime>

#define LOG_TAG "MPP_API"

extern bool playing;
#define SCREEN_WIDTH 1920
#define SCREEN_HEIGHT 1080
#define COL_WINDOW_NUMBER 4
#define ROW_WINDOW_NUMBER 4

void print_time_stamp()
{
    // 获取当前时间点
    auto now = std::chrono::system_clock::now();

    // 将时间点转换为time_t以便转换为本地时间
    std::time_t now_c = std::chrono::system_clock::to_time_t(now);

    // 转换为本地时间
    std::tm now_tm = *std::localtime(&now_c);

    // 打印年月日时分秒
    std::cout << std::put_time(&now_tm, "%Y-%m-%d %H:%M:%S");

    // 获取自纪元以来的毫秒数
    auto milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch());

    // 毫秒数是总毫秒数减去完整秒数后的剩余部分
    std::cout << '.' << std::setfill('0') << std::setw(3) << milliseconds.count() % 1000 << " ";
}

void get_window_info(int processId, int *start_x, int *start_y, int *window_width, int *window_height)
{

    // 计算每个窗口的宽度和高度
    *window_width = SCREEN_WIDTH / COL_WINDOW_NUMBER;
    *window_height = SCREEN_HEIGHT / ROW_WINDOW_NUMBER;

    // 计算窗口在网格中的位置
    int row = processId / COL_WINDOW_NUMBER;
    int col = processId % COL_WINDOW_NUMBER;

    // 计算窗口的起始位置
    *start_x = col * (*window_width);
    *start_y = row * (*window_height);
}

int rga_resize(int src_width, int src_height, int src_format, char *src_buf,
               int dst_width, int dst_height, int dst_format, char *dst_buf)
{

    im_rect src_rect;
    im_rect dst_rect;
    memset(&dst_rect, 0, sizeof(dst_rect));
    memset(&src_rect, 0, sizeof(src_rect));
    // memset(&dst_rect, 0, sizeof(dst_rect));

    // rga_buffer_t src = wrapbuffer_virtualaddr((void *) img_rgb.data, img.cols, img.rows, RK_FORMAT_RGB_888);
    // rga_buffer_t dst = wrapbuffer_virtualaddr((void *) tensor.data, width, height, RK_FORMAT_RGB_888);
    rga_buffer_t src = wrapbuffer_virtualaddr((void *)src_buf, src_width, src_height, src_format);
    rga_buffer_t dst = wrapbuffer_virtualaddr((void *)dst_buf, dst_width, dst_height, dst_format);

    int ret = imcheck(src, dst, src_rect, dst_rect);
    if (IM_STATUS_NOERROR != ret)
    {
        printf("%d, check error! %s", __LINE__, imStrError((IM_STATUS)ret));
        return (-1);
    }

    imresize(src, dst);
    return 0;
}
static int counter = 0;
void mpp_decoder_frame_callback(void *userdata, int width_stride, int height_stride, int width, int height, int format, int fd, void *data)
{
    struct timeval now;
    static struct timeval lastCallback;
    gettimeofday(&now, NULL);
    long timeBetween = now.tv_sec * 1000 + now.tv_usec / 1000 - lastCallback.tv_usec / 1000 - lastCallback.tv_sec * 1000;
    // printf("Time between:%ld ms \n", timeBetween);
    if (timeBetween > 45)
    {
        // printf("Time between:%ld ms \n", timeBetween);
    }

    printf("Time between:%ld ms \n", timeBetween);

    gettimeofday(&lastCallback, NULL);

    usleep(500 * 1000);
}
// 解码后的数据回调函数 没有推理的解码
// 异步方式
// width_stride:1920 height_stride:1088 width:1920 height:1080 format:0 fd:41
void mpp_decoder_frame_callback_1080_works(void *userdata, int width_stride, int height_stride, int width, int height, int format, int fd, void *data)
{
    rknn_app_context_t *ctx = (rknn_app_context_t *)userdata;
    // printf("pid:%d data %p\n", ctx->processId, data);
    int ret = 0;

    struct timeval now;
    struct timeval end;
    struct timeval memcpyTime;
    im_rect src_rect;
    im_rect dst_rect;

    memset(&dst_rect, 0, sizeof(dst_rect));
    memset(&src_rect, 0, sizeof(src_rect));

    char *dstBuf = nullptr;
    char *srcBufCpy = nullptr;

    // rga_buffer_t rgbDstimg;
    rga_buffer_t srcImg;
    rga_buffer_t rgbImg;
    rga_buffer_handle_t srcHandle, dstHandle;

    // memset(&rgbDstimg, 0, sizeof(rgbDstimg));
    memset(&srcImg, 0, sizeof(srcImg));
    memset(&rgbImg, 0, sizeof(rgbImg));

    int srcFormat = RK_FORMAT_YCbCr_420_SP;
    int dstFormat = RK_FORMAT_BGRA_8888;

    int dstHeight = height / ROW_WINDOW_NUMBER;
    int dstWidth = width / COL_WINDOW_NUMBER;
    int srcBufSize = width_stride * height_stride * get_bpp_from_format(srcFormat);
    int dstBufSize = dstHeight * dstWidth * get_bpp_from_format(dstFormat);

    // srcBufSize:3133440 dstBufSize:8355840
    // printf("srcBufSize:%d dstBufSize:%d\n", srcBufSize, dstBufSize);

    if (ctx->processId == 0)
    {
        gettimeofday(&now, NULL);
    }

    // char *srcBufCpy = (char *)malloc(width_stride * height_stride * 2);
    // memcpy(srcBufCpy, data, width_stride * height_stride * 2);

    // 准备工作 获取开始/结束句柄
    srcHandle = importbuffer_virtualaddr(data, srcBufSize);
    dstBuf = (char *)malloc(dstBufSize);
    memset(dstBuf, 0, dstBufSize);
    dstHandle = importbuffer_virtualaddr(dstBuf, dstBufSize);
    if (srcHandle == 0 || dstHandle == 0)
    {
        printf("importbuffer failed!\n");
        goto release_buffer;
    }

    srcImg = wrapbuffer_handle(srcHandle, width_stride, height_stride, srcFormat);
    rgbImg = wrapbuffer_handle(dstHandle, dstWidth, dstHeight, dstFormat);

    // 获取任务
    im_job_handle_t job_handle;
    job_handle = imbeginJob();
    if (job_handle <= 0)
    {
        printf("job begin failed![%d], %s\n", job_handle, imStrError());
        goto release_buffer;
    }

    // dst_rect.x = 0;
    // dst_rect.y = 0;
    // dst_rect.width = width_stride;
    // dst_rect.height = height_stride;
    // dstBuf = (char *)malloc(width_stride * height_stride * 4);

    // rgbImg = wrapbuffer_virtualaddr((void *)dstBuf, width_stride, height_stride, RK_FORMAT_BGRA_8888);

    ret = imcheck(srcImg, rgbImg, {}, dst_rect);
    if (IM_STATUS_NOERROR != ret)
    {
        printf("%d %d, check error! %s \n", ret, __LINE__, imStrError((IM_STATUS)ret));
        imcancelJob(job_handle);
        goto release_buffer;
    }

    ret = improcessTask(job_handle, srcImg, rgbImg, {}, {}, dst_rect, {}, NULL, IM_SYNC);
    if (ret != IM_STATUS_SUCCESS)
    {
        printf("%s job[%d] add left task failed, %s\n", LOG_TAG, job_handle, imStrError((IM_STATUS)ret));
        imcancelJob(job_handle);
        goto release_buffer;
    }

    ret = imendJob(job_handle);
    if (ret != IM_STATUS_SUCCESS)
    {
        printf("%s job[%d] running failed, %s\n", LOG_TAG, job_handle, imStrError((IM_STATUS)ret));
        printf("When job failed, virt ram add is %p data : %p\n", rgbImg.vir_addr, data);
        goto release_buffer;
    }

    if (ctx->processId == 0)
    {
        // draw_hdmi_screen_rgb((uint8_t *)dstBuf, dstBufSize);
        gettimeofday(&end, NULL);
        long timeSpent = end.tv_sec * 1000 + end.tv_usec / 1000 - now.tv_usec / 1000 - now.tv_sec * 1000;
        if (timeSpent > 40)
        {
            printf("Spent: %ld ms\n", timeSpent);
        }
        // printf("Spent:%ld ms\n", end.tv_sec * 1000 + end.tv_usec / 1000 - now.tv_usec / 1000 - now.tv_sec * 1000);
    }

    if (ctx->processId < 16)
    {
        // 4x4屏幕
        draw_hdmi_screen_rgb_dynamic((uint8_t *)dstBuf, dstBufSize, ctx->processId, ROW_WINDOW_NUMBER, COL_WINDOW_NUMBER);
        // draw_hdmi_screen_rgb_nine((uint8_t *)dstBuf, dstBufSize, ctx->processId);
        // draw_hdmi_screen_rgb_quarter((uint8_t *)dstBuf, dstBufSize, ctx->processId);
    }

release_buffer:

    if (srcHandle)
        releasebuffer_handle(srcHandle);

    if (dstHandle)
        releasebuffer_handle(dstHandle);

    if (srcBufCpy != nullptr)
    {
        free(srcBufCpy);
    }

    if (dstBuf != nullptr)
    {
        free(dstBuf);
    }
}

void mpp_decoder_frame_callback_not_so_good(void *userdata, int width_stride, int height_stride, int width, int height, int format, int fd, void *data)
{
    rknn_app_context_t *ctx = (rknn_app_context_t *)userdata;
    int ret = 0;

    // printf("pid :%d \n", ctx->processId);
    // printf("width_stride:%d height_stride:%d width:%d height:%d format:%d fd:%d\n", width_stride, height_stride, width, height, format, fd);
    // static struct timeval start;
    struct timeval now;
    struct timeval end;
    rga_buffer_t rgbDstimg;
    rga_buffer_t originBuf;
    rga_buffer_t rgb_img;
    // struct timeval resizeStart;
    // struct timeval resizeEnd;

    if (ctx->processId == 0)
    {
        gettimeofday(&now, NULL);
        // printf("Time gap:%ld ms\n", now.tv_sec * 1000 + now.tv_usec / 1000 - start.tv_usec / 1000 - start.tv_sec * 1000);
    }

    int dst_width = width_stride / 2;
    int dst_height = height_stride / 2;

    im_job_handle_t job_handle;
    job_handle = imbeginJob();
    if (job_handle <= 0)
    {
        printf("job begin failed![%d], %s\n", job_handle, imStrError());
        return;
    }

    originBuf = wrapbuffer_fd(fd, width, height, RK_FORMAT_YCbCr_420_SP, width_stride, height_stride);

    char *srcBuf = (char *)malloc(width * height * 3 / 2);
    char *dstBuf = (char *)malloc(dst_width * dst_height * 4);

    // memcpy(srcBuf, data, width * height * 3 / 2);
    rgb_img = wrapbuffer_virtualaddr((void *)srcBuf, width, height, RK_FORMAT_YCbCr_420_SP);
    ret = imcopy(originBuf, rgb_img);
    // if (IM_STATUS_NOERROR != ret)
    // {
    //     printf("%d, imcopy error! %s \n", __LINE__, imStrError((IM_STATUS)ret));
    //     goto release_buffer;
    // }

    // rga_buffer_t smallImg = wrapbuffer_virtualaddr((void *)ctx->smallImgBuf, ctx->smallImgWidth, ctx->smallImgHeight, RK_FORMAT_YCbCr_420_SP);
    // cv::Mat origin_mat = cv::Mat::zeros(height, width, CV_8UC3);
    rgbDstimg = wrapbuffer_virtualaddr((void *)dstBuf, dst_width, dst_height, RK_FORMAT_BGRA_8888);

    im_rect dest_rect;
    dest_rect.x = 0;
    dest_rect.y = 0;
    dest_rect.width = dst_width;
    dest_rect.height = dst_height;

    ret = imcheck(rgb_img, rgbDstimg, {}, dest_rect);
    // ret = imcheck(originBuf, rgb_img, {}, dest_rect);
    if (IM_STATUS_NOERROR != ret)
    {
        printf("%d, check error! %s \n", __LINE__, imStrError((IM_STATUS)ret));
        imcancelJob(job_handle);
        goto release_buffer;
    }

    ret = improcessTask(job_handle, rgb_img, rgbDstimg, {}, {}, dest_rect, {}, NULL, IM_SYNC);
    if (ret != IM_STATUS_SUCCESS)
    {
        printf("%s job[%d] add left task failed, %s\n", LOG_TAG, job_handle, imStrError((IM_STATUS)ret));
        imcancelJob(job_handle);
        goto release_buffer;
    }

    ret = imendJob(job_handle);
    if (ret != IM_STATUS_SUCCESS)
    {
        printf("%s job[%d] running failed, %s\n", LOG_TAG, job_handle, imStrError((IM_STATUS)ret));
        goto release_buffer;
    }

    // src = wrapbuffer_fd(mpp_frame_fd, width, height, RK_FORMAT_YCbCr_420_SP, width_stride, height_stride);
    // imcopy(originBuf, rgb_img);

    // if (ctx->processId == 0)
    // {
    // draw_hdmi_screen_rgb_quarter(origin_mat.data, mid_width * mid_height * 4, ctx->processId);
    // draw_hdmi_screen_rgb(origin_mat.data, mid_width * mid_height * 4);
    //}

    if (ctx->processId == 0)
    {
        // draw_hdmi_screen_rgb_quarter((uint8_t *)dstBuf, dst_height * dst_width * 4, 0);
        gettimeofday(&end, NULL);
        long timeSpent = end.tv_sec * 1000 + end.tv_usec / 1000 - now.tv_usec / 1000 - now.tv_sec * 1000;
        printf("Spent: %ld ms\n", timeSpent);
        // gettimeofday(&start, NULL);
    }

    if (ctx->processId == 0)
    {
        // draw_hdmi_screen_rgb_quarter((uint8_t *)dstBuf, dst_height * dst_width * 4, ctx->processId);
    }

release_buffer:

    // printf("mpp_decoder_frame_callback\n");
    // relase
    if (dstBuf != nullptr)
    {
        free(dstBuf);
    }

    if (srcBuf != nullptr)
    {
        free(srcBuf);
    }

    // rga_buffer_t origin;
}

// 解码后的数据回调函数 没有推理的解码
// 异步方式
// width_stride:1920 height_stride:1088 width:1920 height:1080 format:0 fd:41
void mpp_decoder_frame_callback_good(void *userdata, int width_stride, int height_stride, int width, int height, int format, int fd, void *data)
{
    rknn_app_context_t *ctx = (rknn_app_context_t *)userdata;
    int ret = 0;

    // printf("pid :%d \n", ctx->processId);
    // printf("width_stride:%d height_stride:%d width:%d height:%d format:%d fd:%d\n", width_stride, height_stride, width, height, format, fd);
    static struct timeval start;
    struct timeval now;
    struct timeval end;
    struct timeval resizeStart;
    struct timeval resizeEnd;
    if (ctx->processId == 0)
    {
        gettimeofday(&now, NULL);
        // printf("Time gap:%ld ms\n", now.tv_sec * 1000 + now.tv_usec / 1000 - start.tv_usec / 1000 - start.tv_sec * 1000);
    }
    int mid_width = width_stride / 2;
    int mid_height = height_stride / 2;

    /*
     * 1). Create a job handle.
     */
    im_job_handle_t job_handle;
    job_handle = imbeginJob();
    if (job_handle <= 0)
    {
        printf("job begin failed![%d], %s\n", job_handle, imStrError());
        return;
    }

    rga_buffer_t originBuf = wrapbuffer_fd(fd, width, height, RK_FORMAT_YCbCr_420_SP, width_stride, height_stride);
    cv::Mat origin_mat = cv::Mat::zeros(height, width, CV_8UC3);
    // rga_buffer_t rgb_img = wrapbuffer_virtualaddr((void *)origin_mat.data, width, height, RK_FORMAT_RGB_888);
    rga_buffer_t rgb_img = wrapbuffer_virtualaddr((void *)origin_mat.data, width, height, RK_FORMAT_BGR_888);
    im_rect left_rect;
    left_rect.x = 0;
    left_rect.y = 0;
    left_rect.width = width;
    left_rect.height = height;

    ret = imcheck(originBuf, rgb_img, {}, left_rect);
    if (IM_STATUS_NOERROR != ret)
    {
        printf("%d, check error! %s \n", __LINE__, imStrError((IM_STATUS)ret));
        imcancelJob(job_handle);
        goto release_buffer;
    }

    ret = improcessTask(job_handle, originBuf, rgb_img, {}, {}, left_rect, {}, NULL, IM_SYNC);
    if (ret == IM_STATUS_SUCCESS)
    {
        printf("%s job[%d] add left task success!\n", LOG_TAG, job_handle);
    }
    else
    {
        printf("%s job[%d] add left task failed, %s\n", LOG_TAG, job_handle, imStrError((IM_STATUS)ret));
        imcancelJob(job_handle);
        goto release_buffer;
    }

    ret = imendJob(job_handle);
    if (ret == IM_STATUS_SUCCESS)
    {
        printf("%s job[%d] running success!\n", LOG_TAG, job_handle);
    }
    else
    {
        printf("%s job[%d] running failed, %s\n", LOG_TAG, job_handle, imStrError((IM_STATUS)ret));
        goto release_buffer;
    }

    // src = wrapbuffer_fd(mpp_frame_fd, width, height, RK_FORMAT_YCbCr_420_SP, width_stride, height_stride);
    // imcopy(originBuf, rgb_img);

    // if (ctx->processId == 0)
    // {
    // draw_hdmi_screen_rgb_quarter(origin_mat.data, mid_width * mid_height * 4, ctx->processId);
    // draw_hdmi_screen_rgb(origin_mat.data, mid_width * mid_height * 4);
    //}

    if (ctx->processId == 0)
    {
        draw_hdmi_screen_rgb(origin_mat.data, height * width * 3);
        gettimeofday(&end, NULL);
        printf("Trans color and draw screen spent:%ld ms\n", end.tv_sec * 1000 + end.tv_usec / 1000 - now.tv_usec / 1000 - now.tv_sec * 1000);
        // gettimeofday(&start, NULL);
    }

release_buffer:

    printf("mpp_decoder_frame_callback\n");
    // rga_buffer_t origin;
}

// 解码后的数据回调函数 没有推理的解码
void mpp_decoder_frame_callback_rga(void *userdata, int width_stride, int height_stride, int width, int height, int format, int fd, void *data)
{
    rknn_app_context_t *ctx = (rknn_app_context_t *)userdata;
    printf("pid :%d \n", ctx->processId);
    // 计算一下帧率
    static struct timeval start;
    struct timeval now;
    struct timeval end;
    struct timeval resizeStart;
    struct timeval resizeEnd;
    if (ctx->processId == 0)
    {
        gettimeofday(&now, NULL);
        printf("Time gap:%ld ms\n", now.tv_sec * 1000 + now.tv_usec / 1000 - start.tv_usec / 1000 - start.tv_sec * 1000);
    }

    int mid_width = width_stride / 2;
    int mid_height = height_stride / 2;
    // printf("mid_width:%d mid_width:%d\n", mid_width, mid_height);
    char *mid_buf = (char *)malloc(mid_width * mid_height * 3 / 2);
    if (!mid_buf)
    {
        printf("malloc mid_buf failed\n");
        return;
    }

    im_rect src_rect;
    im_rect resize_rect;
    memset(&resize_rect, 0, sizeof(resize_rect));
    memset(&src_rect, 0, sizeof(src_rect));

    rga_buffer_t originBuf = wrapbuffer_fd(fd, width, height, RK_FORMAT_YCbCr_420_SP, width_stride, height_stride);
    // rga_buffer_t src = wrapbuffer_virtualaddr((void *)src_buf, src_width, src_height, src_format);
    rga_buffer_t resizeBuf = wrapbuffer_virtualaddr((void *)mid_buf, mid_width, mid_height, RK_FORMAT_YCbCr_420_SP);

    int ret = imcheck(originBuf, resizeBuf, src_rect, resize_rect);
    if (IM_STATUS_NOERROR != ret)
    {
        printf("%d, check error! %s \n", __LINE__, imStrError((IM_STATUS)ret));
        return;
    }

    int releaesFd = 0;
    gettimeofday(&resizeStart, NULL);
    // resize spent 3ms
    imresize(originBuf, resizeBuf, 0.0, 0.0, 0, 0, &releaesFd);
    while (releaesFd == 0)
    {
        usleep(100);
    }
    gettimeofday(&resizeEnd, NULL);
    printf("resize spent:%ld us\n", resizeEnd.tv_sec * 1000 * 1000 + resizeEnd.tv_usec - resizeStart.tv_usec - resizeStart.tv_sec * 1000 * 1000);
    printf("releaesFd:%d\n", releaesFd);

    // rga_resize(width_stride, height_stride, RK_FORMAT_YCbCr_420_SP, (char *)data,
    // mid_width, mid_height, RK_FORMAT_YCbCr_420_SP, mid_buf);

    // cv::Mat origin_mat = cv::Mat::zeros(height, width, CV_8UC3);

    cv::Mat origin_mat = cv::Mat::zeros(mid_height, mid_width, CV_8UC4);
    rga_buffer_t rgb_img = wrapbuffer_virtualaddr((void *)origin_mat.data, mid_width, mid_height, RK_FORMAT_BGRA_8888);

    // 这里相当于是从origin, 即42SP格式转换成RGB888
    // imcopy(originBuf, rgb_img);
    // 颜色转换 3-4ms
    releaesFd = 0;
    imcopy(resizeBuf, rgb_img, 0, &releaesFd);
    while (releaesFd == 0)
    {
        usleep(100);
    }
    // cv::cvtColor(origin_mat, origin_mat, cv::COLOR_RGB2BGR);

    static bool ifSaved = false;
    if (!ifSaved)
    {
        cv::imwrite("origin.jpg", origin_mat);
        ifSaved = true;
    }

    // if (ctx->processId == 0)
    // {
    draw_hdmi_screen_rgb_quarter(origin_mat.data, mid_width * mid_height * 4, ctx->processId);

    // draw_hdmi_screen_rgb(origin_mat.data, mid_width * mid_height * 4);
    //}
    if (ctx->processId == 0)
    {
        gettimeofday(&end, NULL);
        printf("Process spent:%ld ms\n", end.tv_sec * 1000 + end.tv_usec / 1000 - now.tv_usec / 1000 - now.tv_sec * 1000);
        gettimeofday(&start, NULL);
    }

    // printf("Copy done\n");
    // rga_buffer_t origin;
}

void mpp_decoder_frame_callback_(void *userdata, int width_stride, int height_stride, int width, int height, int format, int fd, void *data)
{

    rknn_app_context_t *ctx = (rknn_app_context_t *)userdata;
    // 计算一下帧率
    static struct timeval start;
    struct timeval now;

    if (ctx->processId == 0)
    {

        gettimeofday(&now, NULL);
        printf("Time gap:%ld ms\n", now.tv_sec * 1000 + now.tv_usec / 1000 - start.tv_usec / 1000 - start.tv_sec * 1000);
        gettimeofday(&start, NULL);
    }

    int ret = 0;
    static int frame_index = 0;
    frame_index++;

    void *mpp_frame = NULL;
    int mpp_frame_fd = 0;
    void *mpp_frame_addr = NULL;
    int enc_data_size;

    rga_buffer_t origin;
    rga_buffer_t src;

    // 编码器准备
    if (ctx->encoder == NULL)
    {
        MppEncoder *mpp_encoder = new MppEncoder();
        MppEncoderParams enc_params;
        memset(&enc_params, 0, sizeof(MppEncoderParams));
        enc_params.width = width;
        enc_params.height = height;
        enc_params.hor_stride = width_stride;
        enc_params.ver_stride = height_stride;
        enc_params.fmt = MPP_FMT_YUV420SP;
        enc_params.type = MPP_VIDEO_CodingAVC;
        mpp_encoder->Init(enc_params, NULL);
        ctx->encoder = mpp_encoder;
    }

    int enc_buf_size = ctx->encoder->GetFrameSize();
    // 3133440
    // printf("enc_buf_size: %d\n", enc_buf_size);
    char *enc_data = (char *)malloc(enc_buf_size);

    mpp_frame = ctx->encoder->GetInputFrameBuffer();
    mpp_frame_fd = ctx->encoder->GetInputFrameBufferFd(mpp_frame);
    mpp_frame_addr = ctx->encoder->GetInputFrameBufferAddr(mpp_frame);

    // 复制到另一个缓冲区，避免修改mpp解码器缓冲区
    // 使用的是RK RGA的格式转换：YUV420SP -> RGB888
    origin = wrapbuffer_fd(fd, width, height, RK_FORMAT_YCbCr_420_SP, width_stride, height_stride);
    // src = wrapbuffer_fd(mpp_frame_fd, width, height, RK_FORMAT_YCbCr_420_SP, width_stride, height_stride);
    cv::Mat origin_mat = cv::Mat::zeros(height, width, CV_8UC3);
    rga_buffer_t rgb_img = wrapbuffer_virtualaddr((void *)origin_mat.data, width, height, RK_FORMAT_RGB_888);
    imcopy(origin, rgb_img);

    static int job_cnt = 0;
    static int result_cnt = 0;

    // printf("Show Image\n");
    // printf("stream_url id:%s \n", ctx->stream_url);
    // 转成brg
    // cv::cvtColor(origin_mat, origin_mat, cv::COLOR_RGB2BGR);
    // char *window_name = (char *)malloc(100);
    // sprintf(window_name, "stream_url id:%s id:%d", ctx->stream_url, ctx->processId);
    // printf("window_name:%s\n", window_name);

    // cv::namedWindow(window_name, cv::WINDOW_NORMAL);
    int start_x = 0;
    int start_y = 0;
    int window_width = 0;
    int window_height = 0;

    // get_window_info(ctx->processId, &start_x, &start_y, &window_width, &window_height);

    // get_window_info(ctx->processId, &start_x, &start_y, &window_width, &window_height);
    // cv::moveWindow(window_name, start_x, start_y);
    // cv::resizeWindow(window_name, window_width, window_height);
    // 直接用opencv show出来
    // cv::imshow(window_name, origin_mat);
    // cv::waitKey(1);

    // 推理完
    // imcopy(rgb_img, src);

RET: // tag
    if (enc_data != nullptr)
    {
        free(enc_data);
    }
}

// 轨道的帧数据回调函数
void API_CALL on_track_frame_out(void *user_data, mk_frame frame)
{
    rknn_app_context_t *ctx = (rknn_app_context_t *)user_data;
    // printf("on_track_frame_out ctx=%p\n", ctx);
    const char *data = mk_frame_get_data(frame);
    ctx->dts = mk_frame_get_dts(frame);
    ctx->pts = mk_frame_get_pts(frame);
    size_t size = mk_frame_get_data_size(frame);
    // printf("size:%zu\n", size);
    // printf("decoder=%p\n", ctx->decoder);
    if (MK_FRAME_FLAG_IS_KEY == mk_frame_get_flags(frame))
    {
        // 打印当前时间戳
        // print_time_stamp();
        // printf("key frame");
    }
    // 解码
    ctx->decoder->decode((uint8_t *)data, size, 0);
    // mk_media_input_frame(ctx->media, frame);
}

void API_CALL on_mk_shutdown_func(void *user_data, int err_code, const char *err_msg, mk_track tracks[], int track_count)
{
    printf("play interrupted: %d %s", err_code, err_msg);
}

void release_media(mk_media *ptr)
{
    if (ptr && *ptr)
    {
        mk_media_release(*ptr);
        *ptr = NULL;
    }
}

void release_pusher(mk_pusher *ptr)
{
    if (ptr && *ptr)
    {
        mk_pusher_release(*ptr);
        *ptr = NULL;
    }
}

void API_CALL on_mk_push_event_func(void *user_data, int err_code, const char *err_msg)
{
    rknn_app_context_t *ctx = (rknn_app_context_t *)user_data;
    if (err_code == 0)
    {
        // push success
        log_info("push %s success!", ctx->push_url);
        printf("push %s success!\n", ctx->push_url);
    }
    else
    {
        log_warn("push %s failed:%d %s", ctx->push_url, err_code, err_msg);
        printf("push %s failed:%d %s\n", ctx->push_url, err_code, err_msg);
        release_pusher(&(ctx->pusher));
    }
}

void API_CALL on_mk_media_source_regist_func(void *user_data, mk_media_source sender, int regist)
{
    // printf("mk_media_source:%x\n", sender);
    rknn_app_context_t *ctx = (rknn_app_context_t *)user_data;
    const char *schema = mk_media_source_get_schema(sender);
    if (strncmp(schema, ctx->push_url, strlen(schema)) == 0)
    {
        // 判断是否为推流协议相关的流注册或注销事件
        printf("schema: %s\n", schema);
        release_pusher(&(ctx->pusher));
        if (regist)
        {
            ctx->pusher = mk_pusher_create_src(sender);
            mk_pusher_set_on_result(ctx->pusher, on_mk_push_event_func, ctx);
            mk_pusher_set_on_shutdown(ctx->pusher, on_mk_push_event_func, ctx);
            //            mk_pusher_publish(ctx->pusher, ctx->push_url);
            log_info("push started!");
            printf("push started!\n");
        }
        else
        {
            log_info("push stoped!");
            printf("push stoped!\n");
        }
        printf("push_url:%s\n", ctx->push_url);
    }
    else
    {
        printf("unknown schema:%s\n", schema);
    }
}
void API_CALL on_mk_play_event_func(void *user_data, int err_code,
                                    const char *err_msg, mk_track tracks[],
                                    int track_count)
{
    rknn_app_context_t *ctx = (rknn_app_context_t *)user_data;
    if (err_code == 0)
    {
        // success
        printf("play success!\n");
        int i;
        ctx->push_url = "rtmp://localhost/live/stream";
        ctx->media = mk_media_create("__defaultVhost__", "live", "test", 0, 0, 0);
        for (i = 0; i < track_count; ++i)
        {
            if (mk_track_is_video(tracks[i]))
            {
                log_info("got video track: %s", mk_track_codec_name(tracks[i]));
                // 监听track数据回调
                mk_media_init_track(ctx->media, tracks[i]);
                // 核心
                mk_track_add_delegate(tracks[i], on_track_frame_out, user_data);
            }
        }
        mk_media_init_complete(ctx->media);
        // mk_media_set_on_regist(ctx->media, on_mk_media_source_regist_func, ctx);
        //      codec_args v_args = {0};
        //      mk_track v_track = mk_track_create(MKCodecH264, &v_args);
        //      mk_media_init_track(ctx->media, v_track);
        //      mk_media_init_complete(ctx->media);
        //      mk_track_unref(v_track);
    }
    else
    {
        printf("play failed: %d %s", err_code, err_msg);
    }
}

int process_video(rknn_app_context_t *ctx, const char *url)
{
    mk_config config;
    memset(&config, 0, sizeof(mk_config));
    config.log_mask = LOG_CONSOLE;
    mk_env_init(&config);
    // mk_rtsp_server_start(rtspPort, 0);
    // mk_rtmp_server_start(1935, 0);
    mk_player player = mk_player_create();
    mk_player_set_on_result(player, on_mk_play_event_func, ctx);
    mk_player_set_on_shutdown(player, on_mk_shutdown_func, ctx);
    mk_player_play(player, url);

    printf("enter any key to exit\n");
    while (playing)
    {
        sleep(1);
    }

    if (player)
    {
        mk_player_release(player);
    }
    return 0;
}
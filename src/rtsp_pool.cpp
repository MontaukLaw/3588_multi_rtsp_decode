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

bool playing = true;

void *rtps_process(void *arg)
{
    int status = 0;
    int ret;

    char *stream_url = (char *)arg; // "rtsp://192.168.1.7:554/stream1"; // 视频流地址

    rknn_app_context_t app_ctx;
    memset(&app_ctx, 0, sizeof(rknn_app_context_t)); // 初始化上下文

    // MPP 解码器
    if (app_ctx.decoder == NULL)
    {
        MppDecoder *decoder = new MppDecoder();           // 创建解码器
        decoder->Init(264, 25, &app_ctx);                 // 初始化解码器
        decoder->SetCallback(mpp_decoder_frame_callback); // 设置回调函数，用来处理解码后的数据
        app_ctx.decoder = decoder;                        // 将解码器赋值给上下文
    }

    printf("app_ctx=%p decoder=%p\n", &app_ctx, app_ctx.decoder);

    // 读取视频流
    process_video(&app_ctx, stream_url);

    printf("waiting finish\n");
    usleep(3 * 1000 * 1000);

    // 释放资源
    if (app_ctx.decoder != nullptr)
    {
        delete (app_ctx.decoder);
        app_ctx.decoder = nullptr;
    }
    if (app_ctx.encoder != nullptr)
    {
        delete (app_ctx.encoder);
        app_ctx.encoder = nullptr;
    }

    return nullptr;
}

int main(int argc, char **argv)
{

    pthread_t rtspPid1;
    pthread_t rtspPid2;
    pthread_t rtspPid3;
    pthread_t rtspPid4;
    pthread_t rtspPid5;
    pthread_t rtspPid6;
    pthread_t rtspPid7;
    pthread_t rtspPid8;

    pthread_create(&rtspPid1, NULL, rtps_process, (void *)"rtsp://192.168.1.7:554/stream1");
    pthread_create(&rtspPid2, NULL, rtps_process, (void *)"rtsp://192.168.1.7:554/stream1");
    pthread_create(&rtspPid3, NULL, rtps_process, (void *)"rtsp://192.168.1.7:554/stream1");
    pthread_create(&rtspPid4, NULL, rtps_process, (void *)"rtsp://192.168.1.7:554/stream1");
    pthread_create(&rtspPid5, NULL, rtps_process, (void *)"rtsp://192.168.1.7:554/stream1");
    pthread_create(&rtspPid6, NULL, rtps_process, (void *)"rtsp://192.168.1.7:554/stream1");
    pthread_create(&rtspPid7, NULL, rtps_process, (void *)"rtsp://192.168.1.7:554/stream1");
    pthread_create(&rtspPid8, NULL, rtps_process, (void *)"rtsp://192.168.1.7:554/stream1");
    getchar();
    getchar();
    playing = false;

    pthread_join(rtspPid1, NULL);
    pthread_join(rtspPid2, NULL);
    pthread_join(rtspPid3, NULL);
    pthread_join(rtspPid4, NULL);
    pthread_join(rtspPid5, NULL);
    pthread_join(rtspPid6, NULL);
    pthread_join(rtspPid7, NULL);
    pthread_join(rtspPid8, NULL);

    return 0;
}

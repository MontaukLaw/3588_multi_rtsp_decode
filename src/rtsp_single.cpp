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

bool playing = true;

typedef struct g_process_config_t
{
    char *stream_url;
    int processId;

} process_config_t;

void *rtps_process(void *arg)
{
    int status = 0;
    int ret;
    process_config_t *config = (process_config_t *)arg;
    // char *stream_url = config; // "rtsp://192.168.1.10:554/stream1"; // 视频流地址

    rknn_app_context_t app_ctx;
    memset(&app_ctx, 0, sizeof(rknn_app_context_t)); // 初始化上下文

    app_ctx.stream_url = (char *)malloc(strlen(config->stream_url) + 1);
    if (app_ctx.stream_url == NULL)
    {
        printf("malloc stream_url failed\n");
        return NULL;
    }
    memcpy(app_ctx.stream_url, config->stream_url, strlen(config->stream_url));
    printf("stream_url=%s process id:%d\n", app_ctx.stream_url, config->processId);
    app_ctx.processId = config->processId;

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
    process_video(&app_ctx, config->stream_url);

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

    hdmi_init();

    pthread_t rtspPid1;
    pthread_t rtspPid2;
    pthread_t rtspPid3;
    pthread_t rtspPid4;
    pthread_t rtspPid5;
    pthread_t rtspPid6;
    pthread_t rtspPid7;
    pthread_t rtspPid8;
    pthread_t rtspPid9;
    pthread_t rtspPid10;
    pthread_t rtspPid11;
    pthread_t rtspPid12;
    pthread_t rtspPid13;
    pthread_t rtspPid14;
    pthread_t rtspPid15;
    pthread_t rtspPid16;

    // process_config_t thread1Config = {"rtsp://192.168.1.10:554/stream1", 0};
    // 1935
    // process_config_t thread1Config = {"rtmp://192.168.1.2:1935/stream", 0};
    // process_config_t thread1Config = {"rtsp://192.168.1.2:8000/stream", 0};
    // process_config_t thread1Config = {"rtsp://192.168.1.10:554/stream1", 0};
    // process_config_t thread2Config = {"rtsp://192.168.1.10:554/stream1", 1};
    // process_config_t thread3Config = {"rtsp://192.168.1.10:554/stream1", 2};
    // process_config_t thread4Config = {"rtsp://192.168.1.10:554/stream1", 3};
    // process_config_t thread5Config = {"rtsp://192.168.1.10:554/stream1", 4};
    // process_config_t thread6Config = {"rtsp://192.168.1.10:554/stream1", 5};
    // process_config_t thread7Config = {"rtsp://192.168.1.10:554/stream1", 6};
    // process_config_t thread8Config = {"rtsp://192.168.1.10:554/stream1", 7};
    // process_config_t thread9Config = {"rtsp://192.168.1.10:554/stream1", 8};
    process_config_t thread1Config = {"rtsp://192.168.1.159:554/stream1", 0};
    process_config_t thread2Config = {"rtsp://192.168.1.159:554/stream1", 1};
    process_config_t thread3Config = {"rtsp://192.168.1.159:554/stream1", 2};
    process_config_t thread4Config = {"rtsp://192.168.1.159:554/stream1", 3};
    process_config_t thread5Config = {"rtsp://192.168.1.159:554/stream1", 4};
    process_config_t thread6Config = {"rtsp://192.168.1.159:554/stream1", 5};
    process_config_t thread7Config = {"rtsp://192.168.1.159:554/stream1", 6};
    process_config_t thread8Config = {"rtsp://192.168.1.159:554/stream1", 7};
    process_config_t thread9Config = {"rtsp://192.168.1.159:554/stream1", 8};
    process_config_t thread10Config = {"rtsp://192.168.1.159:554/stream1", 9};
    process_config_t thread11Config = {"rtsp://192.168.1.159:554/stream1", 10};
    process_config_t thread12Config = {"rtsp://192.168.1.159:554/stream1", 11};
    process_config_t thread13Config = {"rtsp://192.168.1.159:554/stream1", 12};
    process_config_t thread14Config = {"rtsp://192.168.1.159:554/stream1", 13};
    process_config_t thread15Config = {"rtsp://192.168.1.159:554/stream1", 14};
    process_config_t thread16Config = {"rtsp://192.168.1.159:554/stream1", 15};

    pthread_create(&rtspPid1, NULL, rtps_process, (void *)&thread1Config);
    // usleep(1000 * 200);
    sleep(1);
    pthread_create(&rtspPid2, NULL, rtps_process, (void *)&thread2Config);
    // sleep(1000 * 200);
    sleep(1);
    pthread_create(&rtspPid3, NULL, rtps_process, (void *)&thread3Config);
    sleep(1);
    // usleep(1000 * 200);
    pthread_create(&rtspPid4, NULL, rtps_process, (void *)&thread4Config);
    sleep(1);
    // usleep(1000 * 200);
    pthread_create(&rtspPid5, NULL, rtps_process, (void *)&thread5Config);
    sleep(1);
    // usleep(1000 * 200);
    pthread_create(&rtspPid6, NULL, rtps_process, (void *)&thread6Config);
    sleep(1);
    // usleep(1000 * 200);
    pthread_create(&rtspPid7, NULL, rtps_process, (void *)&thread7Config);
    sleep(1);
    // usleep(1000 * 200);
    pthread_create(&rtspPid8, NULL, rtps_process, (void *)&thread8Config);
    sleep(1);
    // usleep(1000 * 200);
    pthread_create(&rtspPid9, NULL, rtps_process, (void *)&thread9Config);

    sleep(1);
    // usleep(1000 * 200);
    pthread_create(&rtspPid10, NULL, rtps_process, (void *)&thread10Config);

    sleep(1);
    // usleep(1000 * 200);
    pthread_create(&rtspPid11, NULL, rtps_process, (void *)&thread11Config);

    sleep(1);
    // usleep(1000 * 200);
    pthread_create(&rtspPid12, NULL, rtps_process, (void *)&thread12Config);

    sleep(1);
    // usleep(1000 * 200);
    pthread_create(&rtspPid13, NULL, rtps_process, (void *)&thread13Config);

    sleep(1);
    // usleep(1000 * 200);
    pthread_create(&rtspPid14, NULL, rtps_process, (void *)&thread14Config);

    sleep(1);
    // usleep(1000 * 200);
    pthread_create(&rtspPid15, NULL, rtps_process, (void *)&thread15Config);

    sleep(1);
    // usleep(1000 * 200);
    pthread_create(&rtspPid16, NULL, rtps_process, (void *)&thread16Config);

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
    pthread_join(rtspPid9, NULL);
    pthread_join(rtspPid10, NULL);
    pthread_join(rtspPid11, NULL);
    pthread_join(rtspPid12, NULL);
    pthread_join(rtspPid13, NULL);
    pthread_join(rtspPid14, NULL);
    pthread_join(rtspPid15, NULL);
    pthread_join(rtspPid16, NULL);

    return 0;
}

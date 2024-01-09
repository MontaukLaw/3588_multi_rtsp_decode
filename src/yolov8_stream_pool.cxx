/*-------------------------------------------
                Includes
-------------------------------------------*/
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

#define OUT_VIDEO_PATH "out.h264"

typedef struct
{
    FILE *out_fp;
    MppDecoder *decoder;
    MppEncoder *encoder;
    mk_media media;
    mk_pusher pusher;
    const char *push_url;
    uint64_t pts;
    uint64_t dts;
} rknn_app_context_t;

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

static Yolov8Custom *yolov8 = nullptr;                 // 模型
static Yolov8ThreadPool *yolov8_thread_pool = nullptr; // 线程池

// 解码后的数据回调函数
void mpp_decoder_frame_callback(void *userdata, int width_stride, int height_stride, int width, int height, int format, int fd, void *data)
{

    rknn_app_context_t *ctx = (rknn_app_context_t *)userdata;

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
    char *enc_data = (char *)malloc(enc_buf_size);

    mpp_frame = ctx->encoder->GetInputFrameBuffer();
    mpp_frame_fd = ctx->encoder->GetInputFrameBufferFd(mpp_frame);
    mpp_frame_addr = ctx->encoder->GetInputFrameBufferAddr(mpp_frame);

    // 复制到另一个缓冲区，避免修改mpp解码器缓冲区
    // 使用的是RK RGA的格式转换：YUV420SP -> RGB888
    origin = wrapbuffer_fd(fd, width, height, RK_FORMAT_YCbCr_420_SP, width_stride, height_stride);
    src = wrapbuffer_fd(mpp_frame_fd, width, height, RK_FORMAT_YCbCr_420_SP, width_stride, height_stride);
    cv::Mat origin_mat = cv::Mat::zeros(height, width, CV_8UC3);
    rga_buffer_t rgb_img = wrapbuffer_virtualaddr((void *)origin_mat.data, width, height, RK_FORMAT_RGB_888);
    imcopy(origin, rgb_img);

    static int job_cnt = 0;
    static int result_cnt = 0;

    // 提交推理任务给线程池
    yolov8_thread_pool->submitTask(origin_mat, job_cnt++);
    std::vector<Detection> objects;
    // 获取推理结果
    auto ret_code = yolov8_thread_pool->getTargetResultNonBlock(objects, result_cnt);
    if (ret_code == NN_SUCCESS)
    {
        result_cnt++;
    }
    else
    {
        goto RET;
    }

    DrawDetections(origin_mat, objects);
    imcopy(rgb_img, src);

    // 推流
    static int dts = 0;
    if (frame_index == 1)
    {
        enc_data_size = ctx->encoder->GetHeader(enc_data, enc_buf_size);
    }
    dts += 40;
    memset(enc_data, 0, enc_buf_size);
    enc_data_size = ctx->encoder->Encode(mpp_frame, enc_data, enc_buf_size);
    printf("enc_data_size=%d\n", enc_data_size);
    printf("pushing...");
    ret = mk_media_input_h264(ctx->media, enc_data, enc_data_size, ctx->dts, ctx->pts);
    if (ret != 1)
    {
        printf("mk_media_input_frame failed\n");
    }

RET: // tag
    if (enc_data != nullptr)
    {
        free(enc_data);
    }
}

void API_CALL on_track_frame_out(void *user_data, mk_frame frame)
{
    rknn_app_context_t *ctx = (rknn_app_context_t *)user_data;
    printf("on_track_frame_out ctx=%p\n", ctx);
    const char *data = mk_frame_get_data(frame);
    ctx->dts = mk_frame_get_dts(frame);
    ctx->pts = mk_frame_get_pts(frame);
    size_t size = mk_frame_get_data_size(frame);
    printf("decoder=%p\n", ctx->decoder);
    ctx->decoder->decode((uint8_t *)data, size, 0);
    //    mk_media_input_frame(ctx->media, frame);
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
    printf("mk_media_source:%x\n", sender);
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

void API_CALL on_mk_play_event_func(void *user_data, int err_code, const char *err_msg, mk_track tracks[],
                                    int track_count)
{
    rknn_app_context_t *ctx = (rknn_app_context_t *)user_data;
    if (err_code == 0)
    {
        // success
        printf("play success!");
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
                mk_track_add_delegate(tracks[i], on_track_frame_out, user_data);
            }
        }
        mk_media_init_complete(ctx->media);
        mk_media_set_on_regist(ctx->media, on_mk_media_source_regist_func, ctx);
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

void API_CALL on_mk_shutdown_func(void *user_data, int err_code, const char *err_msg, mk_track tracks[], int track_count)
{
    printf("play interrupted: %d %s", err_code, err_msg);
}

int process_video_rtsp(rknn_app_context_t *ctx, const char *url)
{
    mk_config config;
    memset(&config, 0, sizeof(mk_config));
    config.log_mask = LOG_CONSOLE;
    mk_env_init(&config);
    mk_rtsp_server_start(554, 0);
    mk_rtmp_server_start(1935, 0);
    mk_player player = mk_player_create();
    mk_player_set_on_result(player, on_mk_play_event_func, ctx);
    mk_player_set_on_shutdown(player, on_mk_shutdown_func, ctx);
    mk_player_play(player, url);

    printf("enter any key to exit\n");
    getchar();

    if (player)
    {
        mk_player_release(player);
    }
    return 0;
}

int main(int argc, char **argv)
{
    int status = 0;
    int ret;

    if (argc != 4)
    {
        printf("Usage: %s <rknn_model> <video_path> <video_type 264/265> \n", argv[0]);
        return -1;
    }
    std::string model_path = (char *)argv[1]; // 模型名称
    char *stream_url = argv[2];               // 视频流地址
    int video_type = atoi(argv[3]);           // 视频流类型：264/265

    yolov8_thread_pool = new Yolov8ThreadPool(); // 创建线程池
    yolov8_thread_pool->setUp(model_path, 12);   // 初始化线程池

    rknn_app_context_t app_ctx;                      // 创建上下文
    memset(&app_ctx, 0, sizeof(rknn_app_context_t)); // 初始化上下文

    // MPP 解码器
    if (app_ctx.decoder == NULL)
    {
        MppDecoder *decoder = new MppDecoder();           // 创建解码器
        decoder->Init(video_type, 30, &app_ctx);          // 初始化解码器
        decoder->SetCallback(mpp_decoder_frame_callback); // 设置回调函数，用来处理解码后的数据
        app_ctx.decoder = decoder;                        // 将解码器赋值给上下文
    }

    if (app_ctx.out_fp == NULL)
    {
        FILE *fp = fopen(OUT_VIDEO_PATH, "w");
        if (fp == NULL)
        {
            printf("open %s error\n", OUT_VIDEO_PATH);
            return -1;
        }
        app_ctx.out_fp = fp;
    }

    printf("app_ctx=%p decoder=%p\n", &app_ctx, app_ctx.decoder);
    // 读取视频流
    process_video_rtsp(&app_ctx, stream_url);

    printf("waiting finish\n");
    usleep(3 * 1000 * 1000);

    // 释放资源
    fflush(app_ctx.out_fp);
    fclose(app_ctx.out_fp);

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

    return 0;
}
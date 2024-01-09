#ifndef __MPP_API_H__
#define __MPP_API_H__

#define TOTAL_WINDOW_NUMBER 2

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
    int processId;
    char *stream_url;
} rknn_app_context_t;

void mpp_decoder_frame_callback(void *userdata, int width_stride, int height_stride, int width, int height, int format, int fd, void *data);

int process_video_rtsp(rknn_app_context_t *ctx, const char *url);

int process_video(rknn_app_context_t *ctx, const char *url);

#endif

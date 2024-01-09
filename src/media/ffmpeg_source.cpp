
#include <unistd.h>
#include "mpi_dec.h"
extern "C"
{
#include <libavformat/avformat.h>
}

static int g_source_width;
static int g_source_height;

static int ready = 0;

extern int decode_simple(MpiDecLoopData *data, AVPacket *av_packet);

void get_source_shape(int *width, int *height)
{
    while (!ready)
    {
        // sleep 1 seconds
        sleep(1);
    }
    *width = g_source_width;
    *height = g_source_height;
}

void deInit(MppPacket *packet, MppFrame *frame, MppCtx ctx, char *buf, MpiDecLoopData data)
{
    if (packet)
    {
        mpp_packet_deinit(packet);
        packet = NULL;
    }

    if (frame)
    {
        mpp_frame_deinit(frame);
        frame = NULL;
    }

    if (ctx)
    {
        mpp_destroy(ctx);
        ctx = NULL;
    }

    if (buf)
    {
        mpp_free(buf);
        buf = NULL;
    }

    if (data.pkt_grp)
    {
        mpp_buffer_group_put(data.pkt_grp);
        data.pkt_grp = NULL;
    }

    if (data.frm_grp)
    {
        mpp_buffer_group_put(data.frm_grp);
        data.frm_grp = NULL;
    }

    if (data.fp_output)
    {
        fclose(data.fp_output);
        data.fp_output = NULL;
    }

    if (data.fp_input)
    {
        fclose(data.fp_input);
        data.fp_input = NULL;
    }
}


int init_ffmpeg_source(const char *filepath)
{
    AVFormatContext *pFormatCtx = NULL;
    AVDictionary *options = NULL;
    AVPacket *av_packet = NULL;

    //    av_register_all();
    printf("avformat_network_init start\n");
    avformat_network_init();
    // 设置读流相关的配置信息
    av_dict_set(&options, "buffer_size", "1024000", 0); // 设置缓存大小
    av_dict_set(&options, "rtsp_transport", "udp", 0);  // 以udp的方式打开,
    av_dict_set(&options, "stimeout", "5000000", 0);    // 设置超时断开链接时间，单位us
    av_dict_set(&options, "max_delay", "500000", 0);    // 设置最大时延

    printf("avformat_alloc_context start\n");

    pFormatCtx = avformat_alloc_context(); // 用来申请AVFormatContext类型变量并初始化默认参数,申请的空间

    printf("avformat_open_input start\n");

    // 打开网络流或文件流
    if (avformat_open_input(&pFormatCtx, filepath, NULL, &options) != 0)
    {
        printf("Couldn't open input stream.\n");
        return 0;
    }

    printf("avformat_find_stream_info start\n");

    // 获取视频文件信息
    if (avformat_find_stream_info(pFormatCtx, NULL) < 0)
    {
        printf("Couldn't find stream information.\n");
        return 0;
    }

    printf("avformat_find_stream_info end\n");

    // 查找码流中是否有视频流
    int videoindex = -1;
    unsigned i = 0;
    for (i = 0; i < pFormatCtx->nb_streams; i++)
        if (pFormatCtx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO)
        {
            videoindex = i;
            break;
        }
    if (videoindex == -1)
    {
        printf("Didn't find a video stream.\n");
        return 0;
    }

    // 获取图像宽高
    int srcWidth = pFormatCtx->streams[videoindex]->codecpar->width;
    int srcHeight = pFormatCtx->streams[videoindex]->codecpar->height;

    g_source_width = srcWidth;
    g_source_height = srcHeight;

    printf("srcWidth is %d, srcHeight is %d\n", srcWidth, srcHeight);

    av_packet = (AVPacket *)av_malloc(sizeof(AVPacket)); // 申请空间，存放的每一帧数据 （h264、h265）
    av_new_packet(av_packet, srcWidth * srcHeight);

    /// setup mpp decoder
    MPP_RET ret = MPP_OK;
    size_t file_size = 0;

    // ctx 是mpp的上下文信息
    MppCtx ctx = NULL;
    // mpi 是mpp的api，调用具体的功能时通过API进行调用
    MppApi *mpi = NULL;

    // 这里的packet 和 frame实际上并没有使用到，可以删除
    // packet 是编码的流数据
    MppPacket packet = NULL;
    // frame 用于存放解码后的数据
    MppFrame frame = NULL;

    MpiCmd mpi_cmd = MPP_CMD_BASE;
    MppParam param = NULL;
    RK_U32 need_split = 1;
    //    MppPollType timeout = 5;

    // paramter for resource malloc
    RK_U32 width = srcWidth;
    RK_U32 height = srcHeight;
    MppCodingType type = MPP_VIDEO_CodingAVC;

    // resources
    char *buf = NULL;
    size_t packet_size = 8 * 1024;
    // packet 和 frame 的数据都是放在 MppBuffer中
    MppBuffer pkt_buf = NULL;
    MppBuffer frm_buf = NULL;

    // 这里定义执行decode循环需要的数据，实际上等于是类里面的成员变量
    MpiDecLoopData g_dec_loop_data;

    mpp_log("mpi_dec_test start\n");
    memset(&g_dec_loop_data, 0, sizeof(g_dec_loop_data));

    // 以下 fp_output 相关代码实际上并不需要， 保留用于测试
    g_dec_loop_data.fp_output = fopen("./tenoutput.yuv", "w+b");
    if (NULL == g_dec_loop_data.fp_output)
    {
        mpp_err("failed to open output file %s\n", "tenoutput.yuv");
        deInit(&packet, &frame, ctx, buf, g_dec_loop_data);
    }

    mpp_log("mpi_dec_test decoder test start w %d h %d type %d\n", width, height, type);

    // 创建一个用于解码的上下文和mpp api
    ret = mpp_create(&ctx, &mpi);

    if (MPP_OK != ret)
    {
        mpp_err("mpp_create failed\n");
        deInit(&packet, &frame, ctx, buf, g_dec_loop_data);
    }

    // NOTE: decoder split mode need to be set before init
    // 设置decoder的模式
    mpi_cmd = MPP_DEC_SET_PARSER_SPLIT_MODE;
    param = &need_split;
    // control是用于控制编解码的行为的api， 下面一个一个调用control实际上就是在一个参数一个参数的进行配置
    ret = mpi->control(ctx, mpi_cmd, param);
    if (MPP_OK != ret)
    {
        mpp_err("mpi->control failed\n");
        deInit(&packet, &frame, ctx, buf, g_dec_loop_data);
    }

    mpi_cmd = MPP_SET_INPUT_BLOCK;
    param = &need_split;
    ret = mpi->control(ctx, mpi_cmd, param);
    if (MPP_OK != ret)
    {
        mpp_err("mpi->control failed\n");
        deInit(&packet, &frame, ctx, buf, g_dec_loop_data);
    }

    ret = mpp_init(ctx, MPP_CTX_DEC, type);
    if (MPP_OK != ret)
    {
        mpp_err("mpp_init failed\n");
        deInit(&packet, &frame, ctx, buf, g_dec_loop_data);
    }

    g_dec_loop_data.ctx = ctx;
    g_dec_loop_data.mpi = mpi;
    g_dec_loop_data.eos = 0;
    g_dec_loop_data.packet_size = packet_size;
    g_dec_loop_data.frame = frame;
    g_dec_loop_data.frame_count = 0;

    ready = 1;

    // 初始化完成后就可以开始从ffmpeg中读取视频流了，然后把视频流扔到解码的代码中。
    while (1)
    {
        if (av_read_frame(pFormatCtx, av_packet) >= 0)
        {
            if (av_packet->stream_index == videoindex)
            {
                printf("--------------\ndata size is: %d\n-------------", av_packet->size);
                decode_simple(&g_dec_loop_data, av_packet);
            }
            if (av_packet != NULL)
                av_packet_unref(av_packet);
            printf("%d", i);
        }
    }
}

#include "zlmedia_worker.h"

#include <csignal>

// 所有的api介绍都可以在对应头文件找到

void API_CALL PushResultCB(void *user_data, int err_code, const char *err_msg)
{
    printf("err_code:%d\n", err_code);
    printf("err_msg:%s\n", err_msg);
}

void API_CALL MediasourceRegistCB(void *user_data, mk_media_source sender, int regist)
{
    printf("mk_media_source:%x\n", sender);
    printf("regist type:%x\n", regist);

    if (regist == 1)
    {
        ZLMediaWorker *ptr = (ZLMediaWorker *)user_data;
        ptr->ZLM_PusherInit();
        ptr->ZLM_PusherStart(ptr->ZLM_GetPushUrl().c_str());
    }
}

// extern int decode_func(void* data, int len, uint32_t pts, uint32_t dts);

void API_CALL on_track_frame_out(void *user_data, mk_frame frame)
{
    void *ptr = (void *)mk_frame_get_data(frame);
    int len = (int)mk_frame_get_data_size(frame);
    uint32_t pts = mk_frame_get_pts(frame);
    uint32_t dts = mk_frame_get_dts(frame);
    //    decode_func(ptr, len, pts, dts);
}

void API_CALL PlayerDataCB(void *user_data, int err_code, const char *err_msg, mk_track tracks[], int track_count)
{
    if (err_code != 0)
    {
        printf("err_code:%d\n", err_code);
        printf("err_msg:%s\n", err_msg);
        return;
    }
    else
    {
        for (int i = 0; i < track_count; ++i)
        {
            if (mk_track_is_video(tracks[i]))
            {
                mk_track_add_delegate(tracks[i], on_track_frame_out, user_data);
            }
        }
    }
}

void API_CALL StreamDataCB(void *user_data, int track_type, int codec_id, void *data, int len, uint32_t dts, uint32_t pts)
{
    // 忽略音频
    if (track_type != 0)
    {
        return;
    }

    ZLMediaWorker *ptr = (ZLMediaWorker *)user_data;
    mk_media pMedia = ptr->ZLM_GetMediaHandle();
    mk_media_input_h264(pMedia, data, len, dts, pts);
    //    decode_func(data, len, pts, dts);
    return;
}

ZLMediaWorker::ZLMediaWorker()
{
}

ZLMediaWorker::~ZLMediaWorker()
{
}

void ZLMediaWorker::ZLM_MediaInit(int width, int height)
{
    mk_config config;
    config.ini = NULL;
    config.ini_is_path = 0;
    config.log_level = 0;
    config.log_mask = LOG_CONSOLE;
    config.ssl = NULL;
    config.ssl_is_path = 1;
    config.ssl_pwd = NULL;
    config.thread_num = 0;

    mk_env_init(&config);
    _mk_MediaHandle = mk_media_create(
        "__defaultVhost__", "live", "camera1",
        0, 0, 1);
    mk_media_init_video(_mk_MediaHandle, 0, width, height, 25, 0);
    mk_media_set_on_regist(_mk_MediaHandle, MediasourceRegistCB, this);
    mk_media_init_complete(_mk_MediaHandle);
}

mk_media ZLMediaWorker::ZLM_GetMediaHandle()
{
    return _mk_MediaHandle;
}

void ZLMediaWorker::ZLM_PusherInit()
{
    _mk_PusherHandle = mk_pusher_create("rtmp", "__defaultVhost__", "live", "camera1");
    mk_pusher_set_on_result(_mk_PusherHandle, PushResultCB, this);
}

void ZLMediaWorker::ZLM_PusherStart(std::string strPushUrl)
{
    mk_pusher_publish(_mk_PusherHandle, strPushUrl.c_str());
}

void ZLMediaWorker::ZLM_SetPushUrl(std::string strPushUrl)
{
    _strPushUrl = strPushUrl;
}

std::string ZLMediaWorker::ZLM_GetPushUrl()
{
    return _strPushUrl;
}

void ZLMediaWorker::ZLM_SetSrcUrl(std::string strSrcUrl)
{
    _strSrcUrl = strSrcUrl;
    _mk_PlayerHandle = mk_player_create();
    mk_player_play(_mk_PlayerHandle, strSrcUrl.c_str());
    mk_player_set_on_result(_mk_PlayerHandle, PlayerDataCB, this);
}

static ZLMediaWorker *g_pZLMediaWorker = nullptr;

void init_zlmediakit(int width, int height, std::string strPushUrl)
{
    g_pZLMediaWorker = new ZLMediaWorker();
    g_pZLMediaWorker->ZLM_SetPushUrl(strPushUrl);
    g_pZLMediaWorker->ZLM_MediaInit(width, height);
}

void stream_data(void *data, int len, uint32_t dts, uint32_t pts)
{
    while (g_pZLMediaWorker == nullptr)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    }
    mk_media pMedia = g_pZLMediaWorker->ZLM_GetMediaHandle();
    mk_media_input_h264(pMedia, data, len, dts, pts);
}
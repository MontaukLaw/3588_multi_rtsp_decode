

#ifndef RK3588_DEMO_ZLMEDIA_WORKER_H
#define RK3588_DEMO_ZLMEDIA_WORKER_H

#include <string>
#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <thread>
#include "mk_media.h"
#include "mk_player.h"
#include "mk_pusher.h"

class ZLMediaWorker
{
public:
    ZLMediaWorker();
    ~ZLMediaWorker();

    void ZLM_MediaInit(int width, int height);
    mk_media ZLM_GetMediaHandle();
    void ZLM_PusherInit();
    void ZLM_PusherStart(std::string strPushUrl);
    void ZLM_SetPushUrl(std::string strPushUrl);
    void ZLM_SetSrcUrl(std::string strSrcUrl);
    std::string ZLM_GetPushUrl();

private:
    mk_player _mk_PlayerHandle;
    mk_media _mk_MediaHandle;
    mk_pusher _mk_PusherHandle;

    std::string _strPushUrl;
    std::string _strSrcUrl;
};

void init_zlmediakit(int width, int height, std::string strPushUrl);

void stream_data(void *data, int len, uint32_t dts, uint32_t pts);

#endif // RK3588_DEMO_ZLMEDIA_WORKER_H

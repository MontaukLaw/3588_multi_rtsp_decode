#include "drm_display.h"

#define OUTPUT_DEVICE_HDMI 1

// static drmModeConnectorPtr lcdConnectorPtr = nullptr;
static drmModeConnectorPtr hdmiConnectorPtr = nullptr;

// static drmModeEncoderPtr lcdEncoderPtr = nullptr;
static drmModeEncoderPtr hdmiEncoderPtr = nullptr;

// static struct sp_crtc *lcdCRPtr;
static struct sp_crtc *hdmiCRPtr;
static uint32_t hdmiCrtcId = 0;
static sp_dev *mDev;
// static drmModeModeInfoPtr lcdModInfoPtr;
static drmModeModeInfoPtr hdmiModInfoPtr;

static void get_connector(uint8_t outpuDevice)
{
    int i, j = 0;
    int ret = 0;

    // connectorPtr, encoderPtr赋值
    // 打印connector的名字
    // connector_type: DRM_MODE_CONNECTOR_DSI : 16
    // connector_type: DRM_MODE_CONNECTOR_HDMIA : 11
    for (j = 0; j < mDev->num_connectors; j++)
    {
        // name 是分辨率信息
        printf("connector name:%s\n", mDev->connectors[j]->modes->name);
        printf("connector_type:%d\n", mDev->connectors[j]->connector_type);
        printf("connector_type_id:%d\n", mDev->connectors[j]->connector_type_id);
        printf("connector status:%d\n", mDev->connectors[j]->connection);
        // 对应不同的输出设备, 指定不同的connector跟encoder
        if (outpuDevice == OUTPUT_DEVICE_HDMI && mDev->connectors[j]->connection == DRM_MODE_CONNECTED)
        {
            if (mDev->connectors[j]->connector_type == DRM_MODE_CONNECTOR_HDMIA)
            {
                hdmiConnectorPtr = mDev->connectors[j];
                return;
            }
        }
    }
}

static void get_crtc()
{
    int j;

    printf("hdmi crtc id:%d\n", hdmiCrtcId);

    for (j = 0; j < mDev->num_crtcs; j++)
    {

        printf("encoderPtr->crtc_id:%d\n", mDev->crtcs[j].crtc->crtc_id);
        printf("mode_valid:%d\n", mDev->crtcs[j].crtc->mode_valid);
        printf("mode_name:%s\n", mDev->crtcs[j].crtc->mode.name);
        if (mDev->crtcs[j].crtc->crtc_id == hdmiCrtcId && mDev->crtcs[j].crtc->mode_valid)
        {
            hdmiCRPtr = &mDev->crtcs[j];
            return;
        }
    }
}

static void get_encoder(uint8_t outpuDevice)
{
    int i;
    for (i = 0; i < mDev->num_encoders; i++)
    {
        if (outpuDevice == OUTPUT_DEVICE_HDMI)
        {
            if (mDev->encoders[i]->encoder_type == DRM_MODE_ENCODER_TMDS)
            {
                hdmiEncoderPtr = mDev->encoders[i];
                hdmiCrtcId = hdmiEncoderPtr->crtc_id;
                return;
            }
        }
    }
}

static int init_hdmi()
{
    int ret = 0;

    // 获取hdmi connector
    get_connector(OUTPUT_DEVICE_HDMI);
    printf("Got connector\n");

    if (!hdmiConnectorPtr)
    {
        printf("failed to get hdmi connector or encoder.\n");
        return -1;
    }

    printf("hdmi connector id:%d\n", hdmiConnectorPtr->connector_id);

    // 获取hdmi encoder
    get_encoder(OUTPUT_DEVICE_HDMI);

    if (!hdmiEncoderPtr)
    {
        printf("failed to get hdmi encoder.\n");
        return -2;
    }

    printf("hdmi encoder id:%d\n", hdmiEncoderPtr->encoder_id);

    // 获取一下显示分辨率之类
    hdmiModInfoPtr = &hdmiConnectorPtr->modes[0];

    // 把connector的encoder id赋值为encoder的id
    hdmiConnectorPtr->encoder_id = hdmiEncoderPtr->encoder_id;

    // 获取lcd crtc
    get_crtc();
    if (!hdmiCRPtr)
    {
        printf("failed to get crtc.\n");
        return -3;
    }

    if (hdmiCRPtr->scanout)
    {
        printf("crtc already in use\n");
        return -4;
    }

    // printf("lcd crtc id:%d\n", lcdCRPtr->crtc->crtc_id);
    printf("hdmi crtc id:%d\n", hdmiCRPtr->crtc->crtc_id);

    // allset
    // 获取bo, 只需要输入分辨率即可.

    hdmiCRPtr->scanout = create_sp_bo(mDev, hdmiModInfoPtr->hdisplay, hdmiModInfoPtr->vdisplay, 24, 32, DRM_FORMAT_XRGB8888, 0);
    if (!hdmiCRPtr->scanout)
    {
        printf("failed to create new scanout bo\n");
        return -6;
    }

    printf("fill test color\n");

    fill_bo(hdmiCRPtr->scanout, 0xff, 0x00, 0x00, 0x0);

    ret = drmModeSetCrtc(mDev->fd, hdmiEncoderPtr->crtc_id, hdmiCRPtr->scanout->fb_id, 0, 0, &hdmiConnectorPtr->connector_id, 1, hdmiModInfoPtr);
    if (ret)
    {
        printf("failed to set crtc mode ret=%d\n", ret);
        return -6;
    }
    hdmiCRPtr->crtc = drmModeGetCrtc(mDev->fd, hdmiCRPtr->crtc->crtc_id);
    memcpy(&hdmiCRPtr->crtc->mode, hdmiModInfoPtr, sizeof(*hdmiModInfoPtr));
    return 0;
}

int hdmi_init()
{
    int ret = 0;
    int i = 0;
    printf("create sp dev\n");
    // 创建显示设备
    mDev = create_sp_dev();
    if (!mDev)
    {
        printf("failed to exec create_sp_dev.\n");
        return -10;
    }

    printf("init_screen\n");

    // 初始化屏幕
    // ret = initialize_screens(mDev);
    ret = init_hdmi();
    // ret = init_screen(OUTPUT_DEVICE_HDMI);
    if (ret != 0)
    {
        printf("failed to exec initialize_screens.\n");
        return -11;
    }
    return 0;
}

void draw_hdmi_screen_rgb_dynamic(uint8_t *data, uint32_t dataSize, uint8_t screenNum, uint8_t rows, uint8_t cols)
{
    uint8_t screenIdx = screenNum;

    if (rows == 0 || cols == 0 || screenIdx >= rows * cols)
    {
        return; // 避免除零错误和数组越界
    }

    uint32_t startRowIdx, startColIdx;
    uint32_t screenWidth = HDMI_SCREEN_WIDTH / cols;
    uint32_t screenHeight = HDMI_SCREEN_HEIGHT / rows;

    // 计算起始行和列索引
    startRowIdx = (screenIdx / cols) * screenHeight; // 根据屏幕编号计算起始行索引
    startColIdx = (screenIdx % cols) * screenWidth;  // 根据屏幕编号计算起始列索引

    uint32_t rowIdx, colIdx;
    uint8_t *dataPtr = data;
    for (rowIdx = startRowIdx; rowIdx < startRowIdx + screenHeight; rowIdx++)
    {
        uint8_t *rowPtr = (uint8_t *)hdmiCRPtr->scanout->map_addr + rowIdx * hdmiCRPtr->scanout->pitch;
        for (colIdx = startColIdx; colIdx < startColIdx + screenWidth; colIdx++)
        {
            uint8_t *pixel = rowPtr + colIdx * 4;
            memcpy(pixel, dataPtr, 4);
            dataPtr += 4;
        }
    }
}

void draw_hdmi_screen_rgb_nine(uint8_t *data, uint32_t dataSize, uint8_t part)
{
    uint32_t startRowIdx, startColIdx;
    uint32_t partWidth = HDMI_SCREEN_WIDTH / 3;
    uint32_t partHeight = HDMI_SCREEN_HEIGHT / 3;

    // 计算起始行和列索引
    startRowIdx = (part / 3) * partHeight; // 根据部分的行来计算起始行索引
    startColIdx = (part % 3) * partWidth;  // 根据部分的列来计算起始列索引

    uint32_t rowIdx, colIdx;
    uint8_t *dataPtr = data;
    for (rowIdx = startRowIdx; rowIdx < startRowIdx + partHeight; rowIdx++)
    {
        uint8_t *rowPtr = (uint8_t *)hdmiCRPtr->scanout->map_addr + rowIdx * hdmiCRPtr->scanout->pitch;
        for (colIdx = startColIdx; colIdx < startColIdx + partWidth; colIdx++)
        {
            uint8_t *pixel = rowPtr + colIdx * 4;
            memcpy(pixel, dataPtr, 4);
            dataPtr += 4;
        }
    }
}

void draw_hdmi_screen_rgb_quarter(uint8_t *data, uint32_t dataSize, uint8_t quarter)
{
    uint32_t startRowIdx, startColIdx;
    switch (quarter)
    {
    case 0: // 左上角
        startRowIdx = 0;
        startColIdx = 0;
        break;
    case 1: // 右上角
        startRowIdx = 0;
        startColIdx = HDMI_SCREEN_WIDTH / 2;
        break;
    case 2: // 左下角
        startRowIdx = HDMI_SCREEN_HEIGHT / 2;
        startColIdx = 0;
        break;
    case 3: // 右下角
        startRowIdx = HDMI_SCREEN_HEIGHT / 2;
        startColIdx = HDMI_SCREEN_WIDTH / 2;
        break;
    default: // 默认为左上角
        startRowIdx = 0;
        startColIdx = 0;
        break;
    }

    uint32_t rowIdx, colIdx;
    uint8_t *dataPtr = data;
    for (rowIdx = startRowIdx; rowIdx < startRowIdx + HDMI_SCREEN_HEIGHT / 2; rowIdx++)
    {
        uint8_t *rowPtr = (uint8_t *)hdmiCRPtr->scanout->map_addr + rowIdx * hdmiCRPtr->scanout->pitch;
        for (colIdx = startColIdx; colIdx < startColIdx + HDMI_SCREEN_WIDTH / 2; colIdx++)
        {
            uint8_t *pixel = rowPtr + colIdx * 4;
            memcpy(pixel, dataPtr, 4);
            dataPtr += 4;
        }
    }
}

void draw_hdmi_screen_rgb(uint8_t *data, uint32_t dataSize)
{

    uint32_t colIdx = 0;
    uint32_t rowIdx = 0;
    uint8_t *dataPtr = data;
    for (rowIdx = 0; rowIdx < HDMI_SCREEN_HEIGHT; rowIdx++)
    {
        uint8_t *rowPtr = (uint8_t *)hdmiCRPtr->scanout->map_addr + rowIdx * hdmiCRPtr->scanout->pitch;
        for (colIdx = 0; colIdx < HDMI_SCREEN_WIDTH; colIdx++)
        {
            uint8_t *pixel = rowPtr + colIdx * 4;
            memcpy(pixel, dataPtr, 4);
            dataPtr += 4;
#if 0
            uint8_t *pixel = rowPtr + colIdx * 4;
            pixel[0] = *dataPtr;
            dataPtr++;
            pixel[1] = *dataPtr;
            dataPtr++;
            pixel[2] = *dataPtr;
            dataPtr++;
            pixel[3] = 0xff;
#endif
        }
    }
}

int main_test()
{
    hdmi_init();
    return 0;
}

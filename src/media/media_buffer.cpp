
#include "media_buffer.h"

#include <deque>
#include <mutex>
#include <thread>
#include <cassert>

class MediaBuffer
{
public:
    MediaBuffer() {}
    ~MediaBuffer();

    void push_src_media(cv::Mat img)
    {
        if (img.empty() || img.rows == 0 || img.cols == 0)
        {
            return;
        }
        std::lock_guard<std::mutex> lock(mutex1_);
        src_buffer_.push_back(img);
        if (src_buffer_.size() > buffer_limit_)
        {
            src_buffer_.pop_front();
        }
    }

    cv::Mat pop_src_media()
    {
        while (src_buffer_.empty())
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
        std::unique_lock<std::mutex> lock(mutex1_);
        cv::Mat img = src_buffer_.front();
        src_buffer_.pop_front();
        return img;
    }

    void push_out_media(cv::Mat img)
    {
        if (img.empty() || img.rows == 0 || img.cols == 0)
        {
            return;
        }
        std::lock_guard<std::mutex> lock(mutex2_);
        out_buffer_.push_back(img);
        if (out_buffer_.size() > buffer_limit_)
        {
            out_buffer_.pop_front();
        }
    }

    cv::Mat pop_out_media()
    {
        while (out_buffer_.empty())
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
        std::unique_lock<std::mutex> lock(mutex2_);
        cv::Mat img = out_buffer_.front();
        out_buffer_.pop_front();
        return img;
    }

private:
    int buffer_limit_ = 10;
    std::mutex mutex1_;
    std::mutex mutex2_;
    std::deque<cv::Mat> src_buffer_;
    std::deque<cv::Mat> out_buffer_;
};

static MediaBuffer *g_media_buffer = nullptr;

void init_media_buffer()
{
    g_media_buffer = new MediaBuffer();
}

void push_src_media(cv::Mat img)
{
    printf("push_src_media.....\n");
    while (g_media_buffer == nullptr)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    }
    g_media_buffer->push_src_media(img);
}

cv::Mat pop_src_media()
{
    printf("pop_src_media.....\n");
    while (g_media_buffer == nullptr)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    }
    return g_media_buffer->pop_src_media();
}

void push_out_media(cv::Mat img)
{
    printf("push_out_media.....\n");
    while (g_media_buffer == nullptr)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    }
    g_media_buffer->push_out_media(img);
}

cv::Mat pop_out_media()
{
    printf("pop_out_media.....\n");
    while (g_media_buffer == nullptr)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    }
    return g_media_buffer->pop_out_media();
}
#include <opencv2/opencv.hpp>


void init_media_buffer();

void push_src_media(cv::Mat img);

cv::Mat pop_src_media();

void push_out_media(cv::Mat img);

cv::Mat pop_out_media();
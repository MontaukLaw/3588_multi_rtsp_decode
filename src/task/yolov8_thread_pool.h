

#ifndef RK3588_DEMO_Yolov8_THREAD_POOL_H
#define RK3588_DEMO_Yolov8_THREAD_POOL_H

#include "yolov8_custom.h"

#include <iostream>
#include <vector>
#include <queue>
#include <map>
#include <thread>
#include <mutex>
#include <condition_variable>

class Yolov8ThreadPool
{
private:
    std::queue<std::pair<int, cv::Mat>> tasks;                   // <id, img>用来存放任务
    std::vector<std::shared_ptr<Yolov8Custom>> Yolov8_instances; // 模型实例
    std::map<int, std::vector<Detection>> results;               // <id, objects>用来存放结果（检测框）
    std::map<int, cv::Mat> img_results;                          // <id, img>用来存放结果（图片）
    std::vector<std::thread> threads;                            // 线程池
    std::mutex mtx1;
    std::mutex mtx2;
    std::condition_variable cv_task;
    bool stop;

    void worker(int id);

public:
    Yolov8ThreadPool();  // 构造函数
    ~Yolov8ThreadPool(); // 析构函数

    nn_error_e setUp(std::string &model_path, int num_threads = 12);             // 初始化
    nn_error_e submitTask(const cv::Mat &img, int id);                           // 提交任务
    nn_error_e getTargetResult(std::vector<Detection> &objects, int id);         // 获取结果（检测框）
    nn_error_e getTargetImgResult(cv::Mat &img, int id);                         // 获取结果（图片）
    nn_error_e getTargetResultNonBlock(std::vector<Detection> &objects, int id); // 获取结果（检测框）非阻塞
    void stopAll();                                                              // 停止所有线程
};

#endif // RK3588_DEMO_Yolov8_THREAD_POOL_H

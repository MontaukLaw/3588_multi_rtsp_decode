# 设置最低版本号
cmake_minimum_required(VERSION 3.11 FATAL_ERROR)
# 设置项目名称
# project(rk3588-demo VERSION 0.0.1 LANGUAGES CXX)
project(rk3588-demo)

# 输出系统信息
message(STATUS "System: ${CMAKE_SYSTEM_NAME} ${CMAKE_SYSTEM_VERSION}")

# 设置编译器
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 设置库架构
set(LIB_ARCH "aarch64")
set(DEVICE_NAME "RK3588")

# 
link_directories(
    mpp_libs
)
#  rknn_api 文件夹路径
set(RKNN_API_PATH ${CMAKE_CURRENT_SOURCE_DIR}/librknn_api)
#  rknn_api include 路径
set(RKNN_API_INCLUDE_PATH ${RKNN_API_PATH}/include)
#  rknn_api lib 路径
set(RKNN_API_LIB_PATH ${RKNN_API_PATH}/${LIB_ARCH}/librknnrt.so)

# 寻找OpenCV库，使用自定义的OpenCV_DIR
set(3RDPARTY_PATH ${CMAKE_CURRENT_SOURCE_DIR}/3rdparty)
set(RGA_DIR ${3RDPARTY_PATH}/rga/${DEVICE_NAME})
set(RGA_LIB ${RGA_DIR}/lib/Linux/${LIB_ARCH}/librga.so)
# set(OpenCV_DIR ${3RDPARTY_PATH}/opencv/opencv-linux-${LIB_ARCH}/share/OpenCV)
find_package(OpenCV REQUIRED)
# 输出OpenCV信息
message(STATUS "include path: ${OpenCV_INCLUDE_DIRS}")
message(STATUS "${OpenCV_VERSION}")

# 用来搜索头文件的目录
include_directories(
    ${OpenCV_INCLUDE_DIRS}
    ${RKNN_API_INCLUDE_PATH}
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/submodules/ZLMediaKit/api/include
    ${RGA_DIR}/include
    ${CMAKE_CURRENT_LIST_DIR}/submodules/mpp/inc
    ${CMAKE_CURRENT_LIST_DIR}/submodules/mpp/osal/inc
    ${CMAKE_CURRENT_LIST_DIR}/submodules/mpp/utils
    ${CMAKE_CURRENT_LIST_DIR}/submodules/mpp/mpp/inc
    ${CMAKE_CURRENT_LIST_DIR}/submodules/mpp/mpp/base/inc
    ${CMAKE_CURRENT_LIST_DIR}/mpp_inc
)

# 构建预处理和后处理库
add_library(nn_process SHARED
            src/process/preprocess.cpp
            src/process/postprocess.cpp
)
# 链接库
target_link_libraries(nn_process
    ${OpenCV_LIBS}
    ${RGA_LIB}
)

# 构建自定义封装API库
add_library(rknn_engine SHARED src/engine/rknn_engine.cpp) 
# 链接库
target_link_libraries(rknn_engine
    ${RKNN_API_LIB_PATH}
)
# yolov85_lib
add_library(yolov8_lib SHARED src/task/yolov8_custom.cpp )
# 链接库
target_link_libraries(yolov8_lib
    rknn_engine
    nn_process
)

# draw_lib
add_library(draw_lib SHARED src/draw/cv_draw.cpp)
# 链接库
target_link_libraries(draw_lib
    ${OpenCV_LIBS}
)

# add_executable(yolov8_stream_pool
#         src/yolov8_stream_pool.cpp
#         src/task/yolov8_thread_pool.cpp
#         src/rkmedia/utils/mpp_decoder.cpp
#         src/rkmedia/utils/mpp_encoder.cpp
# )

# target_link_libraries(yolov8_stream_pool
#         rockchip_mpp # 在mpp_libs下，包含了所有的mpp库
#         utils # 在mpp_libs下，MPP工具库
#         mk_api # 在mpp_libs下，ZLMediaKit API库
#         draw_lib
#         yolov8_lib
# )

##########################
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include) 
# add_executable(rtsp_pool
#         src/rtsp_pool.cpp
#         src/rkmedia/utils/mpp_decoder.cpp
#         src/rkmedia/utils/mpp_encoder.cpp
#         src/mpp_api.cpp
#         src/screen_test.cpp
#         src/bo.c
#         src/dev.c
#         src/modeset.c)

# target_link_libraries(rtsp_pool
#         rockchip_mpp # 在mpp_libs下，包含了所有的mpp库
#         utils # 在mpp_libs下，MPP工具库
#         mk_api # 在mpp_libs下，ZLMediaKit API库
#         draw_lib
#         yolov8_lib
#         drm
# )

############################
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/mpp_inc)      
include_directories(/usr/include/drm)   
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include) 
add_executable(rtsp_single
        src/rtsp_single.cpp
        src/rkmedia/utils/mpp_decoder.cpp
        src/rkmedia/utils/mpp_encoder.cpp
        src/screen_test.cpp
        src/bo.c
        src/dev.c
        src/modeset.c
        src/mpp_api.cpp
        )

target_link_libraries(rtsp_single
        rockchip_mpp # 在mpp_libs下，包含了所有的mpp库
        utils # 在mpp_libs下，MPP工具库
        mk_api # 在mpp_libs下，ZLMediaKit API库
        draw_lib
        yolov8_lib
        drm
)

# find_package(X11 REQUIRED)
# add_executable(screen_demo 
#         src/x11_screen_driver.cpp
#         )

# target_link_libraries(screen_demo X11) 

# add_executable(screen_test 
#                src/screen_test.cpp 
#                src/bo.c
#                src/dev.c
#                src/modeset.c)
# target_link_libraries(screen_test drm)

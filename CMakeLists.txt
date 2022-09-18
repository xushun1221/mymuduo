cmake_minimum_required(VERSION 2.5)
project(mymuduo)

# mymuduo最终编译为so动态库 设置动态库的路径 根目录/lib 目录下
set(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/lib)

# 设置g++编译选项 添加调试信息 设置c++11标准
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -std=c++11 -Wall")

# 定义参与编译的源代码文件 当前目录下的所有文件
aux_source_directory(. SRC_LIST)

# 编译生成mymuduo动态库
add_library(mymuduo SHARED ${SRC_LIST})
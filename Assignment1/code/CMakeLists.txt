# 指定当前项目需要的最低 CMake 版本。
# 这里用 3.10，比教程里的 2.8.11 新一些，支持更现代的 CMake 写法。
cmake_minimum_required(VERSION 3.10)

# 定义项目名称。
# 这个名字会影响生成的工程名，但不一定等于最终 exe 名字。
project(Rasterizer)

# 设置 C++ 标准为 C++17。
# 你的项目源码会按照 C++17 语法来编译。
set(CMAKE_CXX_STANDARD 17)

# 要求编译器必须支持 C++17。
# 如果编译器不支持 C++17，CMake 会直接报错。
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 如果你已经在系统环境变量里设置了 OpenCV_DIR，
# 例如：
# OpenCV_DIR = D:\MyTemp\download\opencv\sources\build
# 那么下面这一行可以不用写。
#
# 如果 CMake 找不到 OpenCV，可以取消下面这行注释，
# 并改成你自己的 OpenCVConfig.cmake 所在目录。
#
# 注意：CMake 路径里推荐使用 /，不要使用 Windows 的 \。
# set(OpenCV_DIR "D:/MyTemp/download/opencv/sources/build")

# 查找 OpenCV。
# REQUIRED 表示必须找到 OpenCV，否则 CMake 配置失败。
#
# 它会根据 OpenCV_DIR 找到 OpenCVConfig.cmake，
# 然后自动设置一些变量，例如：
# OpenCV_INCLUDE_DIRS  OpenCV 头文件路径
# OpenCV_LIBS          OpenCV 需要链接的库
find_package(OpenCV REQUIRED)

# 查找 Eigen3。
# Eigen 是线性代数库，项目里如果使用：
# #include <Eigen/Dense>
# 就需要它。
#
# 如果你已经正确安装 Eigen3，推荐用这种方式。
find_package(Eigen3 REQUIRED)

# 定义最终要生成的可执行程序。
# 第一个参数 Rasterizer 是生成的程序名：
# Rasterizer.exe
#
# 后面列出参与编译的源码文件。
# .cpp 文件会被真正编译。
# .hpp/.h 文件写在这里不是必须的，
# 但写上可以让 IDE/CLion/VSCode 更容易显示项目结构。
add_executable(Rasterizer
    main.cpp
    rasterizer.hpp
    rasterizer.cpp
    Triangle.hpp
    Triangle.cpp
)

# 给 Rasterizer 这个目标添加头文件搜索路径。
#
# PRIVATE 表示这些 include 路径只给 Rasterizer 自己用。
#
# ${OpenCV_INCLUDE_DIRS} 是 find_package(OpenCV) 找到的 OpenCV 头文件目录。
# 这样代码里才能写：
# #include <opencv2/opencv.hpp>
target_include_directories(Rasterizer PRIVATE
    ${OpenCV_INCLUDE_DIRS}
)

# 给 Rasterizer 链接第三方库。
#
# ${OpenCV_LIBS} 是 OpenCVConfig.cmake 提供的库列表。
# 链接它以后，你才能使用：
# cv::imread
# cv::imshow
# cv::Mat
#
# Eigen3::Eigen 是 Eigen 提供的 CMake target。
# Eigen 基本是 header-only 库，不需要链接 .dll，
# 但通过 Eigen3::Eigen 可以自动带上 Eigen 的 include 路径。
target_link_libraries(Rasterizer PRIVATE
    ${OpenCV_LIBS}
    Eigen3::Eigen
)
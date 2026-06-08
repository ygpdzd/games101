cmake_minimum_required(VERSION 3.10)
project(Rasterizer)

find_package(OpenCV REQUIRED)
find_package(Eigen3 REQUIRED)
set(CMAKE_CXX_STANDARD 17)

add_executable(Rasterizer main.cpp rasterizer.hpp rasterizer.cpp global.hpp Triangle.hpp Triangle.cpp Texture.hpp Texture.cpp Shader.hpp OBJ_Loader.h)

# 同时支持 Eigen/Eigen 和 eigen3/Eigen/Eigen 两种包含方式
target_include_directories(Rasterizer PRIVATE
    ${OpenCV_INCLUDE_DIRS}
    ${EIGEN3_INCLUDE_DIR}
)

# 如果存在 eigen3 子目录，也添加到包含路径
get_filename_component(EIGEN_PARENT_DIR "${EIGEN3_INCLUDE_DIR}" DIRECTORY)
if(EXISTS "${EIGEN_PARENT_DIR}/eigen3")
    target_include_directories(Rasterizer PRIVATE
        ${EIGEN_PARENT_DIR}
    )
endif()

target_link_libraries(Rasterizer PRIVATE
    ${OpenCV_LIBS}
    Eigen3::Eigen
)

# target_compile_options(Rasterizer PUBLIC -Wall -Wextra -pedantic)
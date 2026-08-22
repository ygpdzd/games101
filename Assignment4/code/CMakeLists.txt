cmake_minimum_required(VERSION 3.10)
project(BezierCurve)
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
find_package(OpenCV REQUIRED CONFIG)

set(CMAKE_CXX_STANDARD 14)

add_executable(BezierCurve main.cpp)
target_include_directories(BezierCurve PRIVATE
    ${OpenCV_INCLUDE_DIRS}
)
target_link_libraries(BezierCurve ${OpenCV_LIBRARIES})

cmake_minimum_required(VERSION 3.22)
project(eng_thesis CXX)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(OpenCV REQUIRED)
include_directories(${OpenCV_INCLUDE_DIRS})

find_package(PkgConfig REQUIRED)
pkg_check_modules(GPIODXX REQUIRED IMPORTED_TARGET libgpiodcxx)


add_library(MiniPID MiniPID/MiniPID.cpp)
set_property(TARGET MiniPID PROPERTY CXX_STANDARD 20)
target_include_directories(MiniPID PUBLIC MiniPID)

add_executable(cam_reader src/main.cpp src/stereo_camera.cpp)
set_property(TARGET cam_reader PROPERTY CXX_STANDARD 20)
target_include_directories(cam_reader PUBLIC include)
target_link_libraries(cam_reader ${OpenCV_LIBS} MiniPID PkgConfig::GPIODXX)

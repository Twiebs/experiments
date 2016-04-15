cmake_minimum_required(VERSION 3.3)
project(kode_depot)

set(CMAKE_CXX_COMPILER clang++)
set(CMAKE_CXX_FLAGS "-std=c++14 -Wall -Wall")

file(GLOB KD_SOURCES *.cpp)
file(GLOB KD_INCLUDES *.h)
source_group(src FILES ${KD_SOURCES} ${KD_INCLUDES})

add_executable(kd_export kd_export.cpp kd_import.cpp)

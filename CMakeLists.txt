cmake_minimum_required(VERSION 3.15)

project(image_filter VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_subdirectory(osapi)

set(SOURCES src/main.cpp)

add_executable(image_filter ${SOURCES})

target_include_directories(image_filter PRIVATE src)

target_link_libraries(image_filter PRIVATE osapi)

target_compile_features(image_filter PRIVATE cxx_std_20)

find_package(Threads REQUIRED)
target_link_libraries(image_filter PRIVATE osapi Threads::Threads)
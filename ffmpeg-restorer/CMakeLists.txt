cmake_minimum_required(VERSION 3.5)

project(ffmpegrr LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(SOURCE_FILES
  "main.cpp"
  "ffmpeg.cpp"
  "task.cpp"
  "../libs/cpp-exclusive-lock-file/exclusive-lock-file.cpp"
  "../libs/home-dir/home-dir.cpp"
  )


set(HEADER_FILES
  "ffmpeg.h"
  "task.h"
  "../libs/cpp-exclusive-lock-file/exclusive-lock-file.h"
  "../libs/home-dir/home-dir.h"
  "../libs/json/json.hpp"
  )

add_executable(${PROJECT_NAME} ${SOURCE_FILES} ${HEADER_FILES})

target_include_directories(${PROJECT_NAME} PRIVATE "../libs/cpp-exclusive-lock-file")
target_include_directories(${PROJECT_NAME} PRIVATE "../libs/reproc/reproc++/include")
target_include_directories(${PROJECT_NAME} PRIVATE "../libs/home-dir")
target_include_directories(${PROJECT_NAME} PRIVATE "../libs/json")

target_link_libraries(${PROJECT_NAME} reproc++)

install(TARGETS ${PROJECT_NAME})

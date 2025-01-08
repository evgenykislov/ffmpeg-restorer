cmake_minimum_required(VERSION 3.5)

project(ffmpeg-restorer LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(SOURCE_FILES
  "main.cpp"
  "ffmpeg.cpp"
  "task.cpp"
  )


set(HEADER_FILES
  "ffmpeg.h"
  "task.h"
  )

add_executable(${PROJECT_NAME} ${SOURCE_FILES} ${HEADER_FILES})

target_include_directories(${PROJECT_NAME} PRIVATE "../libs/reproc/reproc++/include")

target_link_libraries(${PROJECT_NAME} reproc++)



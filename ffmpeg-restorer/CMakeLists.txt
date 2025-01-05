cmake_minimum_required(VERSION 3.5)

project(ffmpeg-restorer LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(SOURCE_FILES
  "main.cpp"
  "probe.cpp"
  )


set(HEADER_FILES
  "probe.h"
  )

add_executable(${PROJECT_NAME} ${SOURCE_FILES} ${HEADER_FILES})

target_include_directories(${PROJECT_NAME} PRIVATE "../libs/reproc/reproc++/include")

target_link_libraries(${PROJECT_NAME} reproc++)



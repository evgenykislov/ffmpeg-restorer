cmake_minimum_required(VERSION 3.5)

project(FFMpegRestorer)

set(REPROC_INSTALL OFF)
set(REPROC++ ON)

add_subdirectory("libs/reproc")
add_subdirectory("ffmpeg-restorer")

cmake_minimum_required(VERSION 3.5)

project(FFMpegRestorer)

if (APPLICATION_VERSION)
  add_compile_definitions(APPLICATION_VERSION=${APPLICATION_VERSION})
endif()

set(REPROC_INSTALL OFF)
set(REPROC++ ON)

add_subdirectory("libs/reproc")
add_subdirectory("ffmpeg-restorer")

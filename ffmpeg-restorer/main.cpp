#include <cassert>
#include <iostream>

#include "ffmpeg.h"


void PrintHelp() {
  // TODO Implementation
  assert(false);
}


int main(int argc, char** argv) {
  if (argc <= 1) {
    PrintHelp();
    return 1;
  }

  // Добавим новое задание



  if (argc > 1) {
    FFmpeg p;
    size_t duration;
    if (p.RequestDuration(argv[1], duration)) {
      std::cout << "Dur-Dur: " << duration << std::endl;
    }

    std::vector<size_t> a;
    if (p.RequestKeyFrames(argv[1], a)) {
      std::cout << "Dur-Dur: " << duration << std::endl;
    }
  }
  return 0;
}

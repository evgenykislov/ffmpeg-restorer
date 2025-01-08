#include <cassert>
#include <iostream>

#include "ffmpeg.h"
#include "task.h"


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

  Task t;
  if (!t.CreateFromArguments(argc - 1, argv + 1)) {
    std::cerr << "Can't create task from arguments" << std::endl;
    return 1;
  }

  t.Run();


  return 0;
}

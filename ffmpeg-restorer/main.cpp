#include <algorithm>
#include <cassert>
#include <iostream>

#include "ffmpeg.h"
#include "task.h"

const std::string kCommandList = "list";


void PrintHelp() {
  // TODO Implementation
  assert(false);
}


/*! Выполнить команду list - выдать список задач */
void CommandList() {
  auto tasks = Task::GetTasks();
  std::sort(tasks.begin(), tasks.end());
  for (auto it = tasks.begin(); it != tasks.end(); ++it) {
    std::cout << *it << std::endl;
  }
}


int main(int argc, char** argv) {
  if (argc > 1) {
    std::string command = argv[1];

    if (command == kCommandList) {
      CommandList();
    }

    return 0;
  }


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

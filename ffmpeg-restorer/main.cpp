#include <algorithm>
#include <cassert>
#include <iostream>

#include "ffmpeg.h"
#include "task.h"

const std::string kCommandHelp1 = "help";
const std::string kCommandHelp2 = "--help";
const std::string kCommandList = "list";

// clang-format off

const char kHelpMessage[] =
    "ffmpeg-restorer is utility for media files convertation with progress\n"
    "Usage:\n"
    "  ffmpeg-restorer [command [arguments]]\n"
    "Commands:\n"
    "  help, --help - print help\n"
    "  list - print list of tasks\n"
    "Run without command resume tasks, added earlier\n";

// clang-format on

void PrintHelp() { std::cout << kHelpMessage << std::endl; }


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
    } else if (command == kCommandHelp1 || command == kCommandHelp2) {
      PrintHelp();
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

#include <algorithm>
#include <cassert>
#include <iostream>

#include "ffmpeg.h"
#include "task.h"

const std::string kCommandHelp1 = "help";
const std::string kCommandHelp2 = "--help";
const std::string kCommandList = "list";
const std::string kCommandAdd = "add";

// clang-format off

const char kHelpMessage[] =
    "ffmpeg-restorer is utility for media files convertation with progress\n"
    "Usage:\n"
    "  ffmpeg-restorer [command [arguments]]\n"
    "Commands:\n"
    "  add [ffmpeg arguments] - add new task for convertation\n"
    "  help, --help - print help\n"
    "  list - print list of tasks\n"
    "Run without command resume tasks, added earlier\n"
    "Examples:\n"
    "Add task for video stream copy:\n"
    "  ffmpeg-restorer add -i input.mp4 -cv copy output.mp4";

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


/*! Обработать все задачи по-очереди */
void ProcessAllTasks() {
  auto tasks = Task::GetTasks();

  for (auto it = tasks.begin(); it != tasks.end(); ++it) {
    Task t;
    if (t.CreateFromID(*it)) {
      t.Run();
    }
  }
}


int main(int argc, char** argv) {
  if (argc > 1) {
    std::string command = argv[1];

    if (command == kCommandAdd) {
      {
        Task t;
        if (!t.CreateFromArguments(argc - 2, argv + 2)) {
          std::cerr << "Can't create task from arguments" << std::endl;
          return 1;
        }
      }
      ProcessAllTasks();
    } else if (command == kCommandHelp1 || command == kCommandHelp2) {
      PrintHelp();
    } else if (command == kCommandList) {
      CommandList();
    }


    return 0;
  }

  ProcessAllTasks();
  return 0;
}

#include <algorithm>
#include <cassert>
#include <iostream>

#include "ffmpeg.h"
#include "task.h"

const std::string kCommandHelp1 = "help";
const std::string kCommandHelp2 = "--help";
const std::string kCommandList = "list";
const std::string kCommandAdd = "add";
const std::string kCommandFlush = "flush";
const std::string kCommandRemoveAll = "removeall";

// clang-format off

const char kHelpMessage[] =
    "Usage:\n"
    "  ffmpegrr [command [arguments]]\n"
    "Commands:\n"
    "  add [ffmpeg arguments] - add new task for convertation\n"
    "  flush - remove completed (finished) tasks\n"
    "  help, --help - print help\n"
    "  list - print list of tasks\n"
    "  removeall - remove all tasks\n"
    "Run without command resume tasks, added earlier\n"
    "\n"
    "Examples:\n"
    "Add task for video stream copy:\n"
    "  ffmpegrr add -i input.mp4 -cv copy output.mp4";

const char kNoCommandTitleMessage[] =
    "ffmpegrr utility for convertation with progress restoration. Evgeny Kislov, 2025\n"
    "For information run: ffmpegrr --help\n";

const char kTitleMessage[] =
    "ffmpegrr utility for convertation with progress restoration. Evgeny Kislov, 2025\n";

// clang-format on

void PrintHelp() { std::cout << kHelpMessage << std::endl; }


/*! Выполнить команду list - выдать список задач */
void CommandList() {
  auto tasks = Task::GetTasks();
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
    } else {
      std::cerr << "Task " << *it << " is corrupted and will be removed"
                << std::endl;
      Task::DeleteTask(*it);
    }
  }
}


/*! Удалить все завершенные задачи */
void CommandFlush() {
  size_t amount = 0;
  auto tasks = Task::GetTasks();

  for (auto it = tasks.begin(); it != tasks.end(); ++it) {
    Task t;
    if (t.CreateFromID(*it)) {
      if (!t.TaskCompleted()) {
        continue;
      }
    }
    t.Clear();

    Task::DeleteTask(*it);
    ++amount;
  }

  std::cout << "Flushed " << amount << " tasks" << std::endl;
}


/*! Удалить все задачи */
void CommandRemoveAll() {
  auto tasks = Task::GetTasks();

  for (auto it = tasks.begin(); it != tasks.end(); ++it) {
    Task::DeleteTask(*it);
  }

  std::cout << "Removed " << tasks.size() << " tasks" << std::endl;
}


int main(int argc, char** argv) {
  if (argc > 1) {
    std::cout << kTitleMessage << std::endl;
    std::string command = argv[1];

    if (command == kCommandAdd) {
      {
        Task t;
        if (!t.CreateFromArguments(argc - 2, argv + 2)) {
          std::cerr << "Can't create task from arguments" << std::endl;
          return 1;
        }
      }
      std::cout << "Added new task" << std::endl;
      ProcessAllTasks();
    } else if (command == kCommandHelp1 || command == kCommandHelp2) {
      PrintHelp();
    } else if (command == kCommandList) {
      CommandList();
    } else if (command == kCommandFlush) {
      CommandFlush();
    } else if (command == kCommandRemoveAll) {
      CommandRemoveAll();
    } else {
      std::cerr << "Unknown command '" << command << "'" << std::endl;
      return 1;
    }

    return 0;
  }

  // Команда не задана. Делаем обычную конвертацию
  std::cout << kNoCommandTitleMessage << std::endl;
  ProcessAllTasks();
  return 0;
}

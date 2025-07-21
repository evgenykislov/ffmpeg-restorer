#include <algorithm>
#include <cassert>
#include <filesystem>
#include <iostream>
#include <set>
#include <vector>

#include "exclusive-lock-file.h"
#include "home-dir.h"
#include "task.h"


namespace fs = std::filesystem;

const std::string kRunLockFile = "run.lock";

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

std::string g_RunLockPath;

void PrintHelp() { std::cout << kHelpMessage << std::endl; }

bool InitFolders() {
  fs::path hd = fs::absolute(HomeDirLibrary::GetHomeDir());
  if (hd.empty()) {
    std::cerr << "ERROR: There isn't home directory with user files"
              << std::endl;
    return false;
  }
  auto app_dir = hd / kTaskFolder;
  std::error_code err;
  fs::create_directories(app_dir, err);
  if (err) {
    std::cerr << "ERROR: Can't create application directory" << std::endl;
    return false;
  }

  auto lp = app_dir / kRunLockFile;
  g_RunLockPath = lp.string();
  return true;
}


/*! Выполнить команду list - выдать список задач */
void CommandList() {
  auto tasks = Task::GetTasks();
  for (auto it = tasks.begin(); it != tasks.end(); ++it) {
    std::cout << *it << std::endl;
  }
}


/*! Обработать все задачи по-очереди */
void ProcessAllTasks() {
  std::set<size_t> processed;  //!< Уже обработанные/удалённые задачи
  bool runmore = true;
  while (runmore) {
    try {
      runmore = false;

      exclusive_lock_file fl(g_RunLockPath);

      auto tasks = Task::GetTasks();

      for (auto it = tasks.begin(); it != tasks.end(); ++it) {
        if (processed.find(*it) != processed.end()) {
          continue;
        }

        Task t;
        if (t.CreateFromID(*it)) {
          t.Run();
        } else {
          std::cerr << "Task " << *it << " is corrupted and will be removed"
                    << std::endl;
          Task::DeleteTask(*it);
        }
        processed.insert(*it);
        runmore = true;
      }
    } catch (std::runtime_error&) {
      // Уже есть запущенный процесс для обработки задач
      std::cout << "Tasks are already being processed by another process"
                << std::endl;
    }
  }
}


/*! Удалить все завершенные задачи */
void CommandFlush() {
  try {
    exclusive_lock_file fl(g_RunLockPath);

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
  } catch (std::runtime_error&) {
    std::cout << "Flush is blocked while processing tasks" << std::endl;
  }
}


/*! Удалить все задачи */
void CommandRemoveAll() {
  try {
    exclusive_lock_file fl(g_RunLockPath);
    auto tasks = Task::GetTasks();

    for (auto it = tasks.begin(); it != tasks.end(); ++it) {
      Task::DeleteTask(*it);
    }

    std::cout << "Removed " << tasks.size() << " tasks" << std::endl;
  } catch (std::runtime_error&) {
    std::cout << "RemoveAll is blocked while processing tasks" << std::endl;
  }
}


int main(int argc, char** argv) {
  if (!InitFolders()) {
    return 2;
  }

  if (argc > 1) {
    std::cout << kTitleMessage << std::endl;
    std::string command = argv[1];

    if (command == kCommandAdd) {
      try {
        exclusive_lock_file fl(g_RunLockPath);
        Task t;
        if (!t.CreateFromArguments(argc - 2, argv + 2)) {
          std::cerr << "Failed to create new task from specified arguments"
                    << std::endl;
          return 1;
        }
      } catch (std::runtime_error&) {
        std::cout << "Add is blocked while processing tasks" << std::endl;
        return 0;
      }
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

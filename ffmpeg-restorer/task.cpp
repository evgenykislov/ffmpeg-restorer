#include "task.h"

#include <cassert>
#include <iostream>
#include <utility>

#include "ffmpeg.h"


Task::Task(): is_created_(false) {

}

Task::Task(const Task& arg) {
  Copy(*this, arg);
}

Task::Task(Task&& arg) {
  Swap(*this, arg);
}

Task& Task::operator=(const Task& arg) {
  if (this == &arg) {
    return *this;
  }
  Task t(arg);
  Swap(*this, t);
  return *this;
}

Task& Task::operator=(Task&& arg) {
  if (this == &arg) {
    return *this;
  }
  Swap(*this, arg);
  return *this;
}

Task::~Task()
{

}

bool Task::CreateFromArguments(int argc, char** argv) {
  try {
    Clear();
    // Найдём входной и выходной файл. Остальное запомним
    // Входной файл ищём по последовательности аргументов
    // -i filename (и это не последние аргументы)
    // Выходной файл это последний аргумент
    if (argc < 1) { throw std::invalid_argument("a few arguments for convertation"); }
    output_file_ = argv[argc - 1];
    for (int i = 0; i < argc - 1; ++i) {
      std::string v = argv[i];

      if (v == "-i") {
        ++i;
        if (i > (argc - 1)) {
          throw std::invalid_argument("orphan argument -i");
        }
        input_file_ = argv[i];
        continue;
      }

      arguments_.push_back(v);
    }

    if (input_file_.empty() || output_file_.empty()) {
      throw std::invalid_argument("input/output file isn't specified");
    }
    is_created_ = true;
    return true;
  } catch (std::invalid_argument&) {
  } catch (std::bad_alloc&) {
  }
  Clear();
  return false;
}


bool Task::Run() {
  assert(is_created_);
  if (!is_created_) {
    std::cerr << "ERROR: usage of not-created task" << std::endl;
    return false;
  }

  FFmpeg conv;
  return conv.DoConvertation(input_file_, output_file_, 0, 0, arguments_);
}


void Task::Clear() {
  is_created_ = false;
  arguments_.clear();
  input_file_.clear();
  output_file_.clear();
}

void Task::Swap(Task& arg1, Task& arg2) noexcept {
  std::swap(arg1.is_created_, arg2.is_created_);
  std::swap(arg1.arguments_, arg2.arguments_);
  std::swap(arg1.input_file_, arg2.input_file_);
  std::swap(arg1.output_file_, arg2.output_file_);
}


void Task::Copy(Task& arg_to, const Task& arg_from) {
  arg_to.is_created_ = arg_from.is_created_;
  arg_to.arguments_ = arg_from.arguments_;
  arg_to.input_file_ = arg_from.input_file_;
  arg_to.output_file_ = arg_from.output_file_;
}

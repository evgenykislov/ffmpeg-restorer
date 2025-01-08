#include "task.h"

#include <cassert>
#include <chrono>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
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

    if (!GenerateChunks()) {
      throw std::invalid_argument("failed to parse input file");
    }

    if (!GenerateListFile()) {
      throw std::invalid_argument("failed to save list file");
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
  bool result = true;
  for (auto it = chunks_.begin(); it != chunks_.end(); ++it) {
    auto p1 = std::chrono::steady_clock::now();
    conv.DoConvertation(input_file_, it->FileName, it->StartTime, it->Interval, arguments_); // TODO check result
    auto p2 = std::chrono::steady_clock::now();

    size_t prc = (it->StartTime + it->Interval) * 100 / duration_;
    std::cout << prc << "%" << ". Process " << std::chrono::duration_cast<std::chrono::seconds>(p2 - p1).count() << "s" << std::endl;
  }
  conv.DoConcatenation(list_file_, output_file_); // TODO check result

  return true; // TODO check return value
}


void Task::Clear() {
  is_created_ = false;
  arguments_.clear();
  input_file_.clear();
  output_file_.clear();
  list_file_.clear();
}

void Task::Swap(Task& arg1, Task& arg2) noexcept {
  std::swap(arg1.is_created_, arg2.is_created_);
  std::swap(arg1.arguments_, arg2.arguments_);
  std::swap(arg1.input_file_, arg2.input_file_);
  std::swap(arg1.output_file_, arg2.output_file_);
  std::swap(arg1.list_file_, arg2.list_file_);
}


void Task::Copy(Task& arg_to, const Task& arg_from) {
  arg_to.is_created_ = arg_from.is_created_;
  arg_to.arguments_ = arg_from.arguments_;
  arg_to.input_file_ = arg_from.input_file_;
  arg_to.output_file_ = arg_from.output_file_;
  arg_to.list_file_ = arg_from.list_file_;
}

bool Task::GenerateChunks() {
  FFmpeg fm;

  if (!fm.RequestDuration(input_file_, duration_)) {
    return false;
  }

  chunks_.clear();
  size_t pos = 0;

  for (int i = 0; pos < duration_; ++i) {
    // TODO refactor dumb algorithm
    size_t tail = pos + 60000000ULL;

    if (tail > duration_) {
      tail = duration_;
    }

    std::filesystem::path ch_fname;
    std::stringstream suffix;
    suffix << output_file_.stem().string() << "." << std::setw(6) <<
        std::setfill('0') << i << output_file_.extension().string(); // TODO check existance of file
    auto parent = output_file_.parent_path(); // TODO check for empty or other

    Chunk ch;
    ch.FileName = parent / suffix.str();
    ch.StartTime = pos;
    ch.Interval = tail - pos;
    ch.Completed = false;

    chunks_.push_back(ch);

    pos = tail;
  }



  return true;
}

bool Task::GenerateListFile() {
  std::filesystem::path ch_fname;
  std::stringstream suffix;
  suffix << output_file_.stem().string() << ".file_list.txt"; // TODO check existance of file
  auto parent = output_file_.parent_path(); // TODO check for empty or other
  list_file_ = parent / suffix.str();

  std::ofstream f(list_file_.string(), std::ios_base::out | std::ios_base::trunc);
  for (auto it = chunks_.begin(); it != chunks_.end(); ++it) {
    f << "file '" << it->FileName.string() << "'" << std::endl;
  }
  if (!f) {
    return false;
  }
  return true;
}

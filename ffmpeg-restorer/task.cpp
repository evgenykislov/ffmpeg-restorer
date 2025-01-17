#include "task.h"

#include <cassert>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <utility>

#include "ffmpeg.h"
#include "home-dir.h"
#include "json.hpp"

namespace fs = std::filesystem;
using json = nlohmann::json;

const std::string kTaskFolder = ".ffmpeg-restorer";
const std::string kTaskCfgFile = "task.cfg";
const std::string kInterimVideoFile = "video.mkv";
const std::string kInterimDataFile = "data.mkv";

Task::Task(): is_created_(false) { output_file_complete_ = false; }

Task::Task(const Task& arg) { Copy(*this, arg); }

Task::Task(Task&& arg) { Swap(*this, arg); }

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

Task::~Task() {}

bool Task::CreateFromArguments(int argc, char** argv) {
  try {
    Clear();


    // Найдём входной и выходной файл. Остальное запомним
    // Входной файл ищём по последовательности аргументов
    // -i filename (и это не последние аргументы)
    // Выходной файл это последний аргумент
    if (argc < 1) {
      throw std::invalid_argument("a few arguments for convertation");
    }
    output_file_ = argv[argc - 1];
    bool outarg = false;
    for (int i = 0; i < argc - 1; ++i) {
      std::string v = argv[i];

      if (v == "-i") {
        ++i;
        if (i > (argc - 1)) {
          throw std::invalid_argument("orphan argument -i");
        }
        input_file_ = argv[i];
        outarg = true;
        continue;
      }

      if (outarg) {
        output_arguments_.push_back(v);
      } else {
        input_arguments_.push_back(v);
      }
    }

    if (input_file_.empty() || output_file_.empty()) {
      throw std::invalid_argument("input/output file isn't specified");
    }

    // Создаём хранилище для задачи
    size_t id;
    fs::path task_path;
    if (!CreateNewTaskStorage(id, task_path)) {
      throw std::runtime_error("can't create task storage");
    }

    task_cfg_path_ = task_path / kTaskCfgFile;
    interim_video_file_ = task_path / kInterimVideoFile;
    interim_data_file_ = task_path / kInterimDataFile;

    if (!GenerateChunks(task_path)) {
      throw std::invalid_argument("failed to parse input file");
    }

    if (!GenerateListFile()) {
      throw std::invalid_argument("failed to save list file");
    }


    if (!Save()) {
      throw std::runtime_error("can't save task info");
    }

    is_created_ = true;
    return true;
  } catch (std::invalid_argument&) {
  } catch (std::bad_alloc&) {
  }
  Clear();
  return false;
}

bool Task::CreateFromID(size_t id) {
  Clear();
  try {
    fs::path hd = HomeDirLibrary::GetHomeDir();
    if (hd.empty()) {
      std::cerr << "ERROR: There isn't home directory with user files"
                << std::endl;
      return false;
    }

    task_cfg_path_ = hd / kTaskFolder / std::to_string(id) / kTaskCfgFile;

    if (!Load(task_cfg_path_)) {
      std::cerr << "Failed to load task " << id << std::endl;
      return false;
    }

    is_created_ = true;
    return true;
  } catch (std::exception& err) {
    std::cerr << "ERROR: Can't create task: " << err.what() << std::endl;
  }
  Clear();
  return false;
}

bool Task::DeleteTask(size_t id) {
  try {
    fs::path hd = HomeDirLibrary::GetHomeDir();
    if (hd.empty()) {
      std::cerr << "ERROR: There isn't home directory with user files"
                << std::endl;
      return false;
    }

    auto task_path = hd / kTaskFolder / std::to_string(id);
    fs::remove_all(task_path);
    return true;
  } catch (std::exception& err) {
    std::cerr << "ERROR: Can't delete task: " << err.what() << std::endl;
  }
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
    conv.DoConvertation(input_file_, it->FileName, it->StartTime, it->Interval,
        input_arguments_, output_arguments_);  // TODO check result
    auto p2 = std::chrono::steady_clock::now();

    size_t prc = (it->StartTime + it->Interval) * 100 / duration_;
    std::cout
        << prc << "%"
        << ". Process "
        << std::chrono::duration_cast<std::chrono::seconds>(p2 - p1).count()
        << "s" << std::endl;
  }
  conv.DoConcatenation(list_file_, output_file_);  // TODO check result

  return true;  // TODO check return value
}


void Task::Clear() {
  is_created_ = false;
  input_arguments_.clear();
  output_arguments_.clear();
  input_file_.clear();
  output_file_.clear();
  list_file_.clear();
}

std::vector<size_t> Task::GetTasks() {
  try {
    std::vector<size_t> result;
    fs::path hd = HomeDirLibrary::GetHomeDir();
    if (hd.empty()) {
      return result;
    }

    auto task_path = hd / kTaskFolder;
    // переберём все задачи
    for (const auto& item : fs::directory_iterator(task_path)) {
      if (!item.is_directory()) {
        continue;
      }
      auto name = item.path().filename().string();
      long long num = 0;
      try {
        num = std::stoll(name);
        if (num < 0) {
          continue;
        }
      } catch (std::exception&) {
        continue;
      }

      assert(num >= 0);
      result.push_back((size_t)num);
    }

    std::sort(result.begin(), result.end());
    return result;
  } catch (std::exception& err) {
  }
  return {};
}


void Task::Swap(Task& arg1, Task& arg2) noexcept {
  std::swap(arg1.is_created_, arg2.is_created_);
  std::swap(arg1.task_cfg_path_, arg2.task_cfg_path_);
  std::swap(arg1.input_arguments_, arg2.input_arguments_);
  std::swap(arg1.output_arguments_, arg2.output_arguments_);
  std::swap(arg1.input_file_, arg2.input_file_);
  std::swap(arg1.output_file_, arg2.output_file_);
  std::swap(arg1.output_file_complete_, arg2.output_file_complete_);
  std::swap(arg1.list_file_, arg2.list_file_);
  std::swap(arg1.duration_, arg2.duration_);
  std::swap(arg1.chunks_, arg2.chunks_);
  std::swap(arg1.interim_video_file_, arg2.interim_video_file_);
  std::swap(arg1.interim_data_file_, arg2.interim_data_file_);
}


void Task::Copy(Task& arg_to, const Task& arg_from) {
  arg_to.is_created_ = arg_from.is_created_;
  arg_to.task_cfg_path_ = arg_from.task_cfg_path_;
  arg_to.input_arguments_ = arg_from.input_arguments_;
  arg_to.output_arguments_ = arg_from.output_arguments_;
  arg_to.input_file_ = arg_from.input_file_;
  arg_to.output_file_ = arg_from.output_file_;
  arg_to.output_file_complete_ = arg_from.output_file_complete_;
  arg_to.list_file_ = arg_from.list_file_;
  arg_to.duration_ = arg_from.duration_;
  arg_to.chunks_ = arg_from.chunks_;
  arg_to.interim_video_file_ = arg_from.interim_video_file_;
  arg_to.interim_data_file_ = arg_from.interim_data_file_;
}


bool Task::GenerateChunks(const std::filesystem::path& task_path) {
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
    suffix << "chunk_" << std::setw(6) << std::setfill('0') << i << ".mkv";

    Chunk ch;
    ch.FileName = task_path / suffix.str();
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
  suffix << output_file_.stem().string()
         << ".file_list.txt";                // TODO check existance of file
  auto parent = output_file_.parent_path();  // TODO check for empty or other
  list_file_ = parent / suffix.str();

  std::ofstream f(
      list_file_.string(), std::ios_base::out | std::ios_base::trunc);
  for (auto it = chunks_.begin(); it != chunks_.end(); ++it) {
    f << "file '" << it->FileName.string() << "'" << std::endl;
  }
  if (!f) {
    return false;
  }
  return true;
}


bool Task::CreateNewTaskStorage(size_t& id, std::filesystem::path& task_path) {
  try {
    fs::path hd = HomeDirLibrary::GetHomeDir();
    if (hd.empty()) {
      std::cerr << "ERROR: There isn't home directory to store user files"
                << std::endl;
      return false;
    }

    task_path = hd / kTaskFolder;
    fs::create_directory(task_path);
    // Найдём существующий максимальный номер (или ноль)
    id = 0;
    for (const auto& item : fs::directory_iterator(task_path)) {
      if (!item.is_directory()) {
        continue;
      }
      auto name = item.path().filename().string();
      long long num = 0;
      try {
        num = std::stoll(name);
        if (num < 0) {
          continue;
        }
      } catch (std::exception&) {
        continue;
      }

      assert(num >= 0);
      size_t sn = (size_t)num;
      if (id < sn) {
        id = sn;
      }
    }

    ++id;
    task_path /= std::to_string(id);
    fs::create_directory(task_path);
    return fs::is_directory(task_path);
  } catch (std::exception& err) {
    std::cerr << "ERROR: Can't create task: " << err.what() << std::endl;
  }
  return false;
}

bool Task::Save() {
  try {
    // Стандартная форматка
    json j = {{"input", nullptr}, {"output", {{"0", nullptr}}},
        {"interim", {{"video", {{"name", nullptr}, {"complete", false}}},
                        {"data", {{"name", nullptr}, {"complete", false}}}}},
        {"chunks", nullptr}};

    // Заполнение
    j["input"]["0"]["name"] = input_file_.u8string();
    j["input"]["0"]["arguments"] = input_arguments_;
    j["output"]["0"]["name"] = output_file_.u8string();
    j["output"]["0"]["arguments"] = output_arguments_;
    j["output"]["0"]["complete"] = output_file_complete_;

    for (size_t i = 0; i < chunks_.size(); ++i) {
      auto is = std::to_string(i);
      j["chunks"][is]["name"] = chunks_[i].FileName.u8string();
      j["chunks"][is]["start"] = chunks_[i].StartTime;
      j["chunks"][is]["duration"] = chunks_[i].Interval;
      j["chunks"][is]["complete"] = chunks_[i].Completed;
    }

    std::ofstream f(task_cfg_path_, std::ios_base::trunc);
    f << std::setw(2) << j;
    return true;
  } catch (const json::type_error& err) {
    std::cerr << "FORMAT ERROR: " << err.what() << std::endl;
  } catch (std::exception& err) {
    std::cerr << "ERROR: " << err.what() << std::endl;
  }

  return false;
}

bool Task::Load(const std::filesystem::path& task_path_cfg) {
  assert(!is_created_);

  try {
    std::ifstream f(task_path_cfg);
    auto data = json::parse(f);
    std::string strv;
    std::vector<std::string> strarrv;
    strv = data["input"]["0"].value("name", "");
    input_file_ = strv;
    input_arguments_.clear();
    for (auto& el : data["input"]["0"]["arguments"].items()) {
      input_arguments_.push_back(el.value());
    }
    strv = data["output"]["0"].value("name", "");
    output_file_ = strv;
    for (auto& el : data["output"]["0"]["arguments"].items()) {
      output_arguments_.push_back(el.value());
    }
    output_file_complete_ = data["output"]["0"]["complete"];

    chunks_.clear();
    for (auto& el : data["chunks"].items()) {
      auto id = stoull(el.key());
      auto j = el.value();
      Chunk ch;
      std::string strv;
      size_t sizev;
      strv = j.value("name", "");
      ch.FileName = strv;
      ch.StartTime = j.value("start", 0);
      ch.Interval = j.value("duration", 0);
      ch.Completed = j.value("complete", false);
      if (chunks_.size() <= id) {
        chunks_.resize(id + 1);
      }
      chunks_[id] = ch;
    }

  } catch (const json::type_error& err) {
    std::cerr << "FORMAT ERROR: " << err.what() << std::endl;
  } catch (std::exception& err) {
    std::cerr << "ERROR: can't load task configuration: " << err.what()
              << std::endl;
  }
  return false;
}

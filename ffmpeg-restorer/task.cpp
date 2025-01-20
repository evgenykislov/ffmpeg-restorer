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
namespace chr = std::chrono;
using json = nlohmann::json;

const std::string kTaskFolder = ".ffmpeg-restorer";
const std::string kTaskCfgFile = "task.cfg";
const std::string kInterimVideoFile = "video.mkv";
const std::string kInterimDataFile = "data.mkv";
const std::string kInterimListFile = "list.txt";

std::string Microseconds2SecondsString(int value_ms) {
  std::stringstream s;
  s << value_ms / 1000 << "." << std::setw(3) << std::setfill('0')
    << value_ms % 1000;
  return s.str();
}

Task::Task(): is_created_(false) {
  id_ = 0;
  output_file_complete_ = false;
  interim_video_file_complete_ = false;
  interim_data_file_complete_ = false;
  interim_data_file_empty_ = false;
}

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
    auto out_ext = output_file_.extension();
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

    input_file_ = fs::absolute(input_file_);
    output_file_ = fs::absolute(output_file_);
    if (input_file_.empty() || output_file_.empty()) {
      throw std::invalid_argument("input/output file isn't specified");
    }

    // Создаём хранилище для задачи
    fs::path task_path;
    if (!CreateNewTaskStorage(id_, task_path)) {
      throw std::runtime_error("can't create task storage");
    }

    assert(task_path.is_absolute());
    task_cfg_path_ = task_path / kTaskCfgFile;
    interim_video_file_ = task_path / kInterimVideoFile;
    interim_video_file_.replace_extension(out_ext);
    interim_data_file_ = task_path / kInterimDataFile;
    interim_data_file_.replace_extension(out_ext);
    list_file_ = task_path / kInterimListFile;

    if (!GenerateChunks(task_path, out_ext)) {
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
    fs::path hd = fs::absolute(HomeDirLibrary::GetHomeDir());
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

    if (!Validate()) {
      std::cerr << "Task " << id
                << " contains wrong and unrestorable parameters" << std::endl;
      return false;
    }

    id_ = id;
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
    fs::path hd = fs::absolute(HomeDirLibrary::GetHomeDir());
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

  std::cout << "== Task " << id_ << " ==" << std::endl;

  bool split_result = RunSplit();

  std::cout << "Phase 2/4: Video convertation" << std::endl;
  FFmpeg conv;
  bool result = true;
  auto inarg = input_arguments_;
  inarg.push_back("-an");
  inarg.push_back("-sn");
  inarg.push_back("-dn");
  int chunk_counter = 1;
  for (auto it = chunks_.begin(); it != chunks_.end(); ++it, ++chunk_counter) {
    int percent = chunk_counter * 100 / chunks_.size();
    std::cout << "Chunk " << chunk_counter << "/" << chunks_.size() << " ("
              << percent << "%)" << std::flush;
    if (it->Completed) {
      std::cout << " - passed" << std::endl;
      continue;
    }
    auto start = chr::steady_clock::now();
    bool res = conv.DoConvertation(input_file_, it->FileName, it->StartTime,
        it->Interval, inarg, output_arguments_);  // TODO PROCESS !!!
    auto finish = chr::steady_clock::now();
    auto interval =
        chr::duration_cast<chr::milliseconds>(finish - start).count();
    auto is = Microseconds2SecondsString(interval);
    std::cout << " - complete (" << is << " s)";
    if (!res) {
      std::cout << " with error";
    } else {
      it->Completed = true;
      if (!Save()) {
        std::cout << " success, but saving error";
      } else {
        std::cout << " success";
      }
    }
    std::cout << std::endl;
  }


  RunConcatenation();  // TODO echo

  RunMerge();  // TODO echo, check returns, etc

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
    fs::path hd = fs::absolute(HomeDirLibrary::GetHomeDir());
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
  std::swap(arg1.id_, arg2.id_);
  std::swap(arg1.input_arguments_, arg2.input_arguments_);
  std::swap(arg1.output_arguments_, arg2.output_arguments_);
  std::swap(arg1.input_file_, arg2.input_file_);
  std::swap(arg1.output_file_, arg2.output_file_);
  std::swap(arg1.output_file_complete_, arg2.output_file_complete_);
  std::swap(arg1.list_file_, arg2.list_file_);
  std::swap(arg1.duration_, arg2.duration_);
  std::swap(arg1.chunks_, arg2.chunks_);
  std::swap(arg1.interim_video_file_, arg2.interim_video_file_);
  std::swap(
      arg1.interim_video_file_complete_, arg2.interim_video_file_complete_);
  std::swap(arg1.interim_data_file_, arg2.interim_data_file_);
  std::swap(arg1.interim_data_file_complete_, arg2.interim_data_file_complete_);
  std::swap(arg1.interim_data_file_empty_, arg2.interim_data_file_empty_);
}


void Task::Copy(Task& arg_to, const Task& arg_from) {
  arg_to.is_created_ = arg_from.is_created_;
  arg_to.task_cfg_path_ = arg_from.task_cfg_path_;
  arg_to.id_ = arg_from.id_;
  arg_to.input_arguments_ = arg_from.input_arguments_;
  arg_to.output_arguments_ = arg_from.output_arguments_;
  arg_to.input_file_ = arg_from.input_file_;
  arg_to.output_file_ = arg_from.output_file_;
  arg_to.output_file_complete_ = arg_from.output_file_complete_;
  arg_to.list_file_ = arg_from.list_file_;
  arg_to.duration_ = arg_from.duration_;
  arg_to.chunks_ = arg_from.chunks_;
  arg_to.interim_video_file_ = arg_from.interim_video_file_;
  arg_to.interim_video_file_complete_ = arg_from.interim_video_file_complete_;
  arg_to.interim_data_file_ = arg_from.interim_data_file_;
  arg_to.interim_data_file_complete_ = arg_from.interim_data_file_complete_;
  arg_to.interim_data_file_empty_ = arg_from.interim_data_file_empty_;
}


bool Task::GenerateChunks(const std::filesystem::path& task_path,
    const std::filesystem::path& chunk_ext) {
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
    suffix << "chunk_" << std::setw(6) << std::setfill('0') << i
           << chunk_ext.string();

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
  assert(!list_file_.empty());
  if (list_file_.empty()) {
    return false;
  }
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
    fs::path hd = fs::absolute(HomeDirLibrary::GetHomeDir());
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
    assert(input_file_.is_absolute());
    assert(output_file_.is_absolute());

    // Стандартная форматка
    json j = {{"input", nullptr}, {"output", {{"0", nullptr}}},
        {"interim", {{"video", {{"name", nullptr}, {"complete", false}}},
                        {"data", {{"name", nullptr}, {"complete", false},
                                     {"empty", nullptr}}},
                        {"list", {{"name", nullptr}}}}},
        {"chunks", nullptr}};

    // Заполнение
    j["input"]["0"]["name"] = input_file_.u8string();
    j["input"]["0"]["arguments"] = input_arguments_;
    j["output"]["0"]["name"] = output_file_.u8string();
    j["output"]["0"]["arguments"] = output_arguments_;
    j["output"]["0"]["complete"] = output_file_complete_;
    j["interim"]["data"]["name"] = interim_data_file_.u8string();
    j["interim"]["data"]["complete"] = interim_data_file_complete_;
    j["interim"]["data"]["empty"] = interim_data_file_empty_;
    j["interim"]["video"]["name"] = interim_video_file_.u8string();
    j["interim"]["video"]["complete"] = interim_video_file_complete_;
    j["interim"]["list"]["name"] = list_file_.u8string();

    for (size_t i = 0; i < chunks_.size(); ++i) {
      std::string v64;
      auto is = std::to_string(i);
      j["chunks"][is]["name"] = chunks_[i].FileName.u8string();
      j["chunks"][is]["start"] = std::to_string(chunks_[i].StartTime);
      j["chunks"][is]["duration"] = std::to_string(chunks_[i].Interval);
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

    strv = data["interim"]["data"].value("name", "");
    interim_data_file_ = strv;
    interim_data_file_complete_ =
        data["interim"]["data"].value("complete", false);
    interim_data_file_empty_ = data["interim"]["data"].value("empty", false);
    strv = data["interim"]["video"].value("name", "");
    interim_video_file_ = strv;
    interim_video_file_complete_ =
        data["interim"]["video"].value("complete", false);
    strv = data["interim"]["list"].value("name", "");
    list_file_ = strv;

    chunks_.clear();
    for (auto& el : data["chunks"].items()) {
      auto id = stoull(el.key());
      auto j = el.value();
      Chunk ch;
      std::string strv;
      size_t sizev;
      strv = j.value("name", "");
      ch.FileName = strv;
      strv = j.value("start", " ");
      ch.StartTime = std::stoull(strv);
      strv = j.value("duration", " ");
      ch.Interval = std::stoull(strv);
      ch.Completed = j.value("complete", false);
      if (chunks_.size() <= id) {
        chunks_.resize(id + 1);
      }
      chunks_[id] = ch;
    }
    return true;
  } catch (const json::type_error& err) {
    std::cerr << "FORMAT ERROR: " << err.what() << std::endl;
  } catch (std::exception& err) {
    std::cerr << "ERROR: can't load task configuration: " << err.what()
              << std::endl;
  }
  return false;
}

bool Task::Validate() {
  if (input_file_.empty()) {
    return false;
  }
  if (output_file_.empty()) {
    return false;
  }
  if (fs::exists(output_file_)) {
    if (!fs::is_regular_file(output_file_)) {
      std::cout << "WARNING: target " << output_file_
                << " exists and it's not a file" << std::endl;
    }
  } else {
    if (output_file_complete_) {
      // TODO wrong value
    }
    output_file_complete_ = false;
  }

  if (interim_data_file_.empty()) {
    return false;
  }
  if (interim_data_file_empty_) {
    assert(interim_data_file_complete_);
    interim_data_file_complete_ = true;
  } else if (!fs::exists(interim_data_file_)) {
    interim_data_file_complete_ = false;
  }
  if (interim_video_file_.empty()) {
    return false;
  }
  if (!fs::exists(interim_video_file_)) {
    interim_video_file_complete_ = false;
  }

  // Проверим существование отдельных файлов
  for (auto it = chunks_.begin(); it != chunks_.end(); ++it) {
    if (!fs::exists(it->FileName)) {
      it->Completed = false;
    }
  }

  return true;
}


bool Task::RunSplit() {
  std::cout << "Phase 1/4: Extract non-video streams " << std::flush;
  if (interim_data_file_complete_) {
    std::cout << "-- skip" << std::endl;
    return true;
  }
  auto nonvideo = input_arguments_;
  nonvideo.push_back("-vn");
  FFmpeg conv;
  auto start = chr::steady_clock::now();
  auto res = conv.DoConvertation(input_file_, interim_data_file_, {}, {},
      nonvideo, output_arguments_);  // TODO PROCESS !!!
  auto finish = chr::steady_clock::now();
  auto interval = chr::duration_cast<chr::milliseconds>(finish - start).count();
  auto is = Microseconds2SecondsString(interval);

  if (res == FFmpeg::kProcessError) {
    std::cout << "-- complete with error (" << is << " s)" << std::endl;
    return false;
  } else {
    interim_data_file_complete_ = true;
    if (res == FFmpeg::kProcessEmpty) {
      interim_data_file_empty_ = true;
      std::cout << " (empty output) ";
    }
    if (!Save()) {
      std::cout << "-- complete, but saving error (" << is << " s)"
                << std::endl;
      return false;
    }

    std::cout << "-- complete (" << is << " s)" << std::endl;
  }
  return true;
}

bool Task::RunConcatenation() {
  std::cout << "Phase 3/4: Concatenate video chunks ... " << std::flush;
  if (interim_video_file_complete_) {
    std::cout << "passed" << std::endl;
    return true;
  }
  FFmpeg conv;
  auto start = chr::steady_clock::now();
  bool res = conv.DoConcatenation(
      list_file_, interim_video_file_);  // TODO check result
  auto finish = chr::steady_clock::now();
  auto interval = chr::duration_cast<chr::milliseconds>(finish - start).count();
  auto is = Microseconds2SecondsString(interval);
  if (!res) {
    std::cout << "failed ";
  } else {
    interim_video_file_complete_ = true;
    res = Save();
    if (!res) {
      std::cout << " complete, but saving error ";
    }

    std::cout << " success ";
  }
  std::cout << "(" << is << " s)" << std::endl;
  return res;
}


bool Task::RunMerge() {
  std::cout << "Phase 4/4: Merge streams ... " << std::flush;
  if (output_file_complete_) {
    std::cout << "skip" << std::endl;
    return true;
  }

  if (interim_data_file_empty_) {
    // Простое копирование файла с видео
    std::cout << " (copying) ";
    if (fs::copy_file(interim_video_file_, output_file_,
            fs::copy_options::overwrite_existing)) {
      output_file_complete_ = true;
      if (Save()) {
        std::cout << " success" << std::endl;
        return true;
      }
      std::cout << " complete, but saving error " << std::endl;
    } else {
      std::cout << " failed" << std::endl;
    }
    return false;
  }

  FFmpeg conv;
  auto start = chr::steady_clock::now();
  bool res = conv.MergeVideoAndData(
      interim_video_file_, interim_data_file_, output_file_);
  auto finish = chr::steady_clock::now();
  auto interval = chr::duration_cast<chr::milliseconds>(finish - start).count();
  auto is = Microseconds2SecondsString(interval);
  if (!res) {
    std::cout << "failed ";
  } else {
    output_file_complete_ = true;
    res = Save();
    if (!res) {
      std::cout << " complete, but saving error ";
    }

    std::cout << " success ";
  }
  std::cout << "(" << is << " s)" << std::endl;
  return res;
}

#include "ffmpeg.h"

#include <cassert>
#include <iostream>
#include <limits>
#include <sstream>
#include <string>
#include <vector>

#include <reproc++/drain.hpp>
#include <reproc++/reproc.hpp>


bool Str2Duration(const std::string& str, size_t& value) {
  // Note: use format from
  // https://ffmpeg.org/ffmpeg-utils.html#time-duration-syntax
  //  [-][HH:]MM:SS[.m...]
  //  HH expresses the number of hours, MM the number of minutes for a maximum
  //  of 2 digits, and SS the number of seconds for a maximum of 2 digits. The m
  //  at the end expresses decimal value for SS.

  // TODO check parts of seconds is not microseconds only
  // TODO check time without hours

  if (str.empty()) {
    return false;
  }
  unsigned long long hour, minute, second, micro;
  auto res = std::sscanf(
      str.c_str(), "%llu:%llu:%llu.%llu", &hour, &minute, &second, &micro);
  if (res != 4) {
    return false;
  }
  value = hour * 3600 + minute * 60 + second;
  value = value * 1000000 + micro;
  return true;
}

std::string Duration2Str(size_t value) {
  std::stringstream str;

  size_t whole_sec = value / 1000000ULL;
  size_t microsec = value % 1000000ULL;

  unsigned int hour = whole_sec / 3600;
  unsigned int tail = whole_sec % 3600;
  unsigned int minutes = tail / 60;
  unsigned int seconds = tail % 60;

  str << std::setw(2) << std::setfill('0') << hour << ":" << std::setw(2)
      << std::setfill('0') << minutes << ":" << std::setw(2)
      << std::setfill('0') << seconds << "." << std::setw(6)
      << std::setfill('0') << microsec;
  return str.str();
}


bool RunApplication(const std::string& application,
    const std::vector<std::string>& arguments, std::string& output,
    std::string& errout) {
  try {
    std::vector<const char*> raw_args;
    raw_args.reserve(arguments.size() + 2);
    raw_args.push_back(application.c_str());
    for (auto it = arguments.begin(); it != arguments.end(); ++it) {
      raw_args.push_back(it->c_str());
    }
    raw_args.push_back(nullptr);

    reproc::process proc;
    reproc::options opt;
    reproc::sink::string sout(output);
    reproc::sink::string serr(errout);
    std::error_code err;

    opt.redirect.parent = false;
    opt.redirect.in.type = reproc::redirect::discard;
    opt.redirect.out.type = reproc::redirect::pipe;
    opt.redirect.err.type = reproc::redirect::pipe;

    err = proc.start(raw_args.data(), opt);
    if (err == std::errc::no_such_file_or_directory) {
      std::cerr << "Error: " << application << " not found. Install ffmpeg pack"
                << std::endl;
      return false;
    } else if (err) {
      std::cerr << "Error: " << err.category().name() << ":" << err.value()
                << std::endl;
      return false;
    }

    err = reproc::drain(proc, sout, serr);
    if (err) {
      std::cerr << "Error: " << err.category().name() << ":" << err.value()
                << std::endl;
      return false;
    }

    int status = 0;
    std::tie(status, err) = proc.wait(reproc::infinite);
    if (err) {
      std::cerr << application
                << " finished with error: " << err.category().name() << ":"
                << err.value() << std::endl;
      return false;
    }
    if (status != 0) {
      // std::cerr << application << " returns error " << status << std::endl;
      return false;
    }

    return true;
  } catch (std::exception& err) {
    std::cerr << "Error: " << err.what() << std::endl;
  }
  return false;
}


FFmpeg::FFmpeg() {}

bool FFmpeg::ParseDuration(const std::string& value, size_t& duration_mcs) {
  try {
    std::istringstream ss(value);
    constexpr size_t kUndef = std::numeric_limits<size_t>::max();
    duration_mcs = kUndef;
    for (std::string line; std::getline(ss, line);) {
      if (duration_mcs != kUndef) {
        return false;
      }  // Несколько значений
      if (!Str2Duration(line, duration_mcs)) {
        return false;
      }
    }
    return true;
  } catch (std::exception&) {
  }
  return false;
}

bool FFmpeg::ParseKeyFrames(
    const std::string& value, std::vector<size_t>& key_frames) {
  assert(false);  // TODO Not implemented
  return false;
}

bool FFmpeg::DetectEmptyOutput(const std::string& error_description) {
  std::string kFirstPart = "Output file";
  std::string kSecondPart = "does not contain any stream";

  std::stringstream err(error_description);
  std::string line;
  while (std::getline(err, line)) {
    if (line.find(kFirstPart) == line.npos) {
      continue;
    }
    if (line.find(kSecondPart) == line.npos) {
      continue;
    }
    return true;
  }

  return false;
}

FFmpeg::~FFmpeg() {}

bool FFmpeg::RequestDuration(
    const std::filesystem::path& fname, size_t& duration_mcs) {
  try {
    std::vector<std::string> arguments = {"-v", "error", "-show_entries",
        "format=duration", "-sexagesimal", "-of",
        "default=noprint_wrappers=1:nokey=1"};
    arguments.push_back(fname.string());

    std::string output;
    std::string errout;
    if (!RunApplication("ffprobe", arguments, output, errout)) {
      return false;
    }

    if (!ParseDuration(output, duration_mcs)) {
      std::cerr << "ffprobe returns unknown format value" << std::endl;
      return false;
    }
    return true;
  } catch (std::exception& err) {
    std::cerr << "Error: " << err.what() << std::endl;
  }
  return false;
}

bool FFmpeg::RequestKeyFrames(
    const std::string& fname, std::vector<size_t>& key_frames) {
  try {
    std::vector<std::string> arguments = {"-select_streams", "v", "-skip_frame",
        "nokey", "-show_frames", "-show_entries",
        "frame=pkt_pts_time,pict_type", "-sexagesimal"};
    arguments.push_back(fname);

    std::string output;
    std::string errout;
    if (!RunApplication("ffprobe", arguments, output, errout)) {
      return false;
    }

    std::cout << "Receive information about key frames with size "
              << output.size() << " symbols" << std::endl;

    //    if (!ParseDuration(output, duration_mcs)) {
    //      std::cerr << "ffprobe returns unknown format value" << std::endl;
    //      return false;
    //    }
    return true;
  } catch (std::exception& err) {
    std::cerr << "Error: " << err.what() << std::endl;
  }
  return false;
}


FFmpeg::ProcessResult FFmpeg::DoConvertation(std::filesystem::path input_file,
    std::filesystem::path output_file, std::optional<size_t> start_time,
    std::optional<size_t> interval,
    const std::vector<std::string>& input_arguments,
    const std::vector<std::string>& output_arguments) {
  try {
    std::vector<std::string> raw_args = {"-hide_banner", "-y"};
    if (start_time && interval) {
      raw_args.push_back("-ss");
      raw_args.push_back(Duration2Str(start_time.value()));
      raw_args.push_back("-t");
      raw_args.push_back(Duration2Str(interval.value()));
    } else {
      assert(!start_time);
      assert(!interval);
    }
    raw_args.insert(std::end(raw_args), std::begin(input_arguments),
        std::end(input_arguments));
    raw_args.push_back("-i");
    raw_args.push_back(input_file.string());
    raw_args.insert(std::end(raw_args), std::begin(output_arguments),
        std::end(output_arguments));
    raw_args.push_back(output_file.string());

    std::string output;
    std::string errout;
    if (!RunApplication("ffmpeg", raw_args, output, errout)) {
      // Конвертация выполнилась с ошибкой. Но возможен вариант пустого файла
      if (DetectEmptyOutput(errout)) {
        return kProcessEmpty;
      }

      // std::cout << "ERROR:" << std::endl << errout << std::endl;
      return kProcessError;
    }

    return kProcessSuccess;
  } catch (std::exception& err) {
    std::cerr << "Error: " << err.what() << std::endl;
  }
  return kProcessError;
}

bool FFmpeg::MergeVideoAndData(std::filesystem::path video_file,
    std::filesystem::path data_file,
    std::filesystem::__cxx11::path output_file) {
  try {
    // clang-format off
    std::vector<std::string> raw_args = {"-hide_banner", "-y",
        // Source #0 (video)
        "-i", video_file.string(),
        // Source #1 (audio, subtitle, data)
        "-i", data_file.string(),
        // Copy video
        "-map", "0:v?", "-c:v", "copy",
        // Copy audio
        "-map", "1:a?", "-c:a", "copy",
        // Copy subtitle
        "-map", "1:s?", "-c:s", "copy",
        // Copy data
        "-map", "1:d?", "-c:d", "copy",
        // Destination
        output_file.string()};
    // clang-format on

    std::string output;
    std::string errout;
    if (!RunApplication("ffmpeg", raw_args, output, errout)) {
      std::cout << "ERROR:" << std::endl << errout << std::endl;
      return false;
    }
    return true;
  } catch (std::exception& err) {
    std::cerr << "Error: " << err.what() << std::endl;
  }
  return false;
}

bool FFmpeg::DoConcatenation(
    std::filesystem::path list_file, std::filesystem::path output_file) {
  try {
    std::vector<std::string> raw_args = {"-hide_banner", "-safe", "0", "-f",
        "concat", "-y", "-i", list_file.string(), "-c", "copy",
        output_file.string()};

    std::string output;
    std::string errout;
    if (!RunApplication("ffmpeg", raw_args, output, errout)) {
      std::cout << "ERROR:" << std::endl << errout << std::endl;

      return false;
    }

    return true;
  } catch (std::exception& err) {
    std::cerr << "Error: " << err.what() << std::endl;
  }
  return false;
}

std::string FFmpeg::RequestStreamInfo(const std::filesystem::path& file) {
  try {
    std::vector<std::string> raw_args = {
        "-v", "quiet", "-print_format", "json", "-show_streams", file.string()};
    std::string output;
    std::string errout;
    if (!RunApplication("ffprobe", raw_args, output, errout)) {
      std::cout << "ERROR:" << std::endl << errout << std::endl;
      return {};
    }

    return output;
  } catch (std::exception& err) {
    std::cerr << "Error: " << err.what() << std::endl;
  }
  return {};
}

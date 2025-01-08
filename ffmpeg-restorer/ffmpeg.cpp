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
  // Note: use format from https://ffmpeg.org/ffmpeg-utils.html#time-duration-syntax
  //  [-][HH:]MM:SS[.m...]
  //  HH expresses the number of hours, MM the number of minutes for a maximum of 2 digits, and SS the number of seconds for a maximum of 2 digits. The m at the end expresses decimal value for SS.

  // TODO check parts of seconds is not microseconds only
  // TODO check time without hours

  if (str.empty()) { return false; }
  unsigned long long hour, minute, second, micro;
  auto res = std::sscanf(str.c_str(), "%llu:%llu:%llu.%llu", &hour, &minute,
    &second, &micro);
  if (res != 4) { return false; }
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

  str << std::setw(2) << std::setfill('0') << hour << ":" << std::setw(2) <<
      std::setfill('0') << minutes << ":" << std::setw(2) << std::setfill('0') <<
      seconds << "." << std::setw(6) << std::setfill('0') << microsec;
  return str.str();
}


bool RunApplication(const std::string& application, const std::vector<std::string>& arguments, std::string& output, std::string& errout) {
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
      std::cerr << "Error: ffprobe not found. Install ffmpeg pack" << std::endl;
      return false;
    } else if (err) {
      std::cerr << "Error: " << err.category().name() << ":" << err.value() << std::endl;
      return false;
    }

    err = reproc::drain(proc, sout, serr);
    if (err) {
      std::cerr << "Error: " << err.category().name() << ":" << err.value() << std::endl;
      return false;
    }

    int status = 0;
    std::tie(status, err) = proc.wait(reproc::infinite);
    if (err) {
      std::cerr << "ffprobe finished with error: " << err.category().name() <<
          ":" << err.value() << std::endl;
      return false;
    }
    if (status != 0) {
      std::cerr << "ffprobe returns error " << status << std::endl;
      return false;
    }

    return true;
  } catch (std::exception& err) {
    std::cerr << "Error: " << err.what() << std::endl;
  }
  return false;
}


FFmpeg::FFmpeg()
{

}

bool FFmpeg::ParseDuration(const std::string& value, size_t& duration_mcs) {
  try {
    std::istringstream ss(value);
    constexpr size_t kUndef = std::numeric_limits<size_t>::max();
    duration_mcs = kUndef;
    for (std::string line; std::getline(ss, line);) {
      if (duration_mcs != kUndef) { return false; } // Несколько значений
      if (!Str2Duration(line, duration_mcs)) { return false; }
    }
    return true;
  } catch (std::exception&) {
  }
  return false;
}

bool FFmpeg::ParseKeyFrames(const std::string& value, std::vector<size_t>& key_frames) {
  assert(false); // TODO Not implemented
  return false;
}

FFmpeg::~FFmpeg() {
}

bool FFmpeg::RequestDuration(const std::filesystem::path& fname, size_t& duration_mcs) {
  try {
    std::vector<std::string> arguments = {"-v", "error",
      "-show_entries", "format=duration", "-sexagesimal", "-of",
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

bool FFmpeg::RequestKeyFrames(const std::string& fname, std::vector<size_t>& key_frames) {
  try {
    std::vector<std::string> arguments = { "-select_streams", "v", "-skip_frame",
        "nokey", "-show_frames", "-show_entries", "frame=pkt_pts_time,pict_type",
        "-sexagesimal" };
    arguments.push_back(fname);

    std::string output;
    std::string errout;
    if (!RunApplication("ffprobe", arguments, output, errout)) {
      return false;
    }

    std::cout << "Receive information about key frames with size " << output.size() << " symbols" << std::endl;

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


bool FFmpeg::DoConvertation(std::filesystem::path input_file,
    std::filesystem::path output_file,
    size_t start_time, size_t interval,
    const std::vector<std::string>& arguments) {

  try {
    std::vector<std::string> raw_args = {"-hide_banner", "-y", "-ss",
        Duration2Str(start_time), "-t", Duration2Str(interval),
        "-i", input_file.string(), };
    raw_args.insert(std::end(raw_args), std::begin(arguments), std::end(arguments));
    raw_args.push_back(output_file.string());

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

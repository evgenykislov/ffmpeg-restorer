#include "probe.h"

#include <iostream>
#include <limits>
#include <sstream>
#include <string>
#include <vector>

#include <reproc++/drain.hpp>
#include <reproc++/reproc.hpp>


bool Str2Duration(const std::string& str, size_t& value) {
  if (str.empty()) { return false; }
  unsigned long long hour, minute, second, micro;
  auto res = std::sscanf(str.c_str(), "%llu:%llu:%llu.%llu", &hour, &minute,
    &second, &micro);
  if (res != 4) { return false; }
  value = hour * 3600 + minute * 60 + second;
  value = value * 1000000 + micro;
  return true;
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
    reproc::sink::string sout(output);
    reproc::sink::string serr(errout);
    std::error_code err;

    err = proc.start(raw_args.data());
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


Probe::Probe()
{

}

bool Probe::ParseDuration(const std::string& value, size_t& duration_mcs) {
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

bool Probe::ParseKeyFrames(const std::string& value, std::vector<size_t>& key_frames) {

}

Probe::~Probe() {
}

bool Probe::RequestDuration(const std::string& fname, size_t& duration_mcs) {
  try {
    std::vector<std::string> arguments = {"-v", "error",
      "-show_entries", "format=duration", "-sexagesimal", "-of",
      "default=noprint_wrappers=1:nokey=1"};
    arguments.push_back(fname);

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

bool Probe::RequestKeyFrames(const std::string& fname, std::vector<size_t>& key_frames) {
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

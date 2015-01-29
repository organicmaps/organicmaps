// This header file is provided here for the convenience of Alex from Minsk, Belarus, of using FSQ.

#include <chrono>
#include <iostream>
#include <thread>

#include "fsq.h"
#include "../Bricks/file/file.h"

struct MinimalisticProcessor {
  template <typename T_TIMESTAMP>
  fsq::FileProcessingResult OnFileReady(const fsq::FileInfo<T_TIMESTAMP>& file, T_TIMESTAMP /*now*/) {
    std::cerr << file.full_path_name << std::endl << bricks::ReadFileAsString(file.full_path_name) << std::endl;
    return fsq::FileProcessingResult::Success;
  }
};

int main() {
  MinimalisticProcessor processor;
  fsq::FSQ<fsq::Config<MinimalisticProcessor>> fsq(processor, ".");
  fsq.PushMessage("Hello, World!\n");
  fsq.ForceProcessing();
  std::this_thread::sleep_for(std::chrono::milliseconds(50));
}

#ifndef FSQ_EXPONENTIAL_RETRY_STRATEGY_H
#define FSQ_EXPONENTIAL_RETRY_STRATEGY_H

#include <iostream>

#include <string>
#include <atomic>
#include <random>

#include "exception.h"
#include "../Bricks/time/chrono.h"

namespace fsq {
namespace strategy {

// To be used primarily by the unit test to have different rand seeds.
// TODO(dkorolev): Move to Bricks.
struct StatefulRandSeed {
  static int GetRandSeed() {
    static std::atomic_int seed(static_cast<int>(bricks::time::Now()));
    return ++seed;
  }
};

// Exponential retry strategy for the processing of finalized files.
// On `Success`, processes files as they arrive without any delays.
// On `Unavaliable`, retries after an amount of time drawn from an exponential distribution
// with the mean defaulting to 15 minutes, min defaulting to 1 minute and max defaulting to 24 hours.
template <typename FILE_SYSTEM_FOR_RETRY_STRATEGY>
class ExponentialDelayRetryStrategy {
 public:
  typedef FILE_SYSTEM_FOR_RETRY_STRATEGY T_FILE_SYSTEM;
  struct DistributionParams {
    double mean, min, max;
    DistributionParams(double mean, double min, double max) : mean(mean), min(min), max(max) {}
    DistributionParams() = default;
    DistributionParams(const DistributionParams&) = default;
    DistributionParams& operator=(const DistributionParams&) = default;
  };
  explicit ExponentialDelayRetryStrategy(const T_FILE_SYSTEM& file_system, const DistributionParams& params)
      : file_system_(file_system),
        last_update_time_(bricks::time::Now()),
        time_to_be_ready_to_process_(last_update_time_),
        params_(params),
        rng_(StatefulRandSeed::GetRandSeed()),
        distribution_(1.0 / params.mean) {}
  explicit ExponentialDelayRetryStrategy(const T_FILE_SYSTEM& file_system,
                                         const double mean = 15 * 60 * 1e3,
                                         const double min = 60 * 1e3,
                                         const double max = 24 * 60 * 60 * 1e3)
      : ExponentialDelayRetryStrategy(file_system, DistributionParams(mean, min, max)) {}
  void AttachToFile(const std::string& filename) {
    if (!filename.empty()) {
      persistence_filename_ = filename;
      // First, resume delay, is possible.
      // Then, save it to a) ensure the file exists, and b) update its timestamp.
      ResumeStateFromFile();
      SaveStateToFile();
    } else {
      throw FSQException();
    }
  }
  // OnSuccess(): Clear all retry delays, cruising at full speed.
  void OnSuccess() {
    last_update_time_ = bricks::time::Now();
    time_to_be_ready_to_process_ = last_update_time_;
    if (!persistence_filename_.empty()) {
      SaveStateToFile();
    }
  }
  // OnFailure(): Set or update all retry delays.
  void OnFailure() {
    const bricks::time::EPOCH_MILLISECONDS now = bricks::time::Now();
    double random_delay;
    do {
      random_delay = distribution_(rng_);
    } while (!(random_delay >= params_.min && random_delay <= params_.max));
    time_to_be_ready_to_process_ = std::max(
        time_to_be_ready_to_process_, now + static_cast<bricks::time::MILLISECONDS_INTERVAL>(random_delay));
    last_update_time_ = now;
    if (!persistence_filename_.empty()) {
      SaveStateToFile();
    }
  }
  bool ShouldWait(bricks::time::MILLISECONDS_INTERVAL* output_wait_ms) {
    const bricks::time::EPOCH_MILLISECONDS now = bricks::time::Now();
    if (now >= time_to_be_ready_to_process_) {
      return false;
    } else {
      *output_wait_ms = time_to_be_ready_to_process_ - now;
      return true;
    }
  }

 protected:
  void ResumeStateFromFile() const {
    using typename bricks::time::EPOCH_MILLISECONDS;
    const EPOCH_MILLISECONDS now = bricks::time::Now();
    try {
      const std::string contents = std::move(file_system_.ReadFileAsString(persistence_filename_));
      constexpr size_t w = bricks::strings::FixedSizeSerializer<EPOCH_MILLISECONDS>::size_in_bytes;
      // File format is "${update_time} ${time_to_be_ready_to_process}".
      if (contents.length() == w * 2 + 1 && contents[w] == ' ') {
        EPOCH_MILLISECONDS last_update_time;
        EPOCH_MILLISECONDS time_to_be_ready_to_process;
        bricks::strings::UnpackFromString(contents.substr(0, w), last_update_time);
        bricks::strings::UnpackFromString(contents.substr(w + 1, w), time_to_be_ready_to_process);
        if (last_update_time <= now) {
          last_update_time_ = now;
          time_to_be_ready_to_process_ = std::max(time_to_be_ready_to_process_, time_to_be_ready_to_process);
        } else {
          // TODO(dkorolev): Log an error message, time skew detected, not resuming from file,
          // overwriting it with "no delay" instead.
        }
      } else {
        // TODO(dkorolev): Log an error message, file format is incorrect.
      }
    } catch (const bricks::FileException&) {
      // TODO(dkorolev): Log an error message, could not read the file.
    }
  }
  void SaveStateToFile() const {
    using bricks::strings::PackToString;
    try {
      file_system_.WriteStringToFile(
          persistence_filename_.c_str(),
          PackToString(last_update_time_) + ' ' + PackToString(time_to_be_ready_to_process_));
    } catch (const bricks::FileException&) {
      // TODO(dkorolev): Log an error message, could not read the file.
    }
  }

 private:
  const T_FILE_SYSTEM& file_system_;
  mutable typename bricks::time::EPOCH_MILLISECONDS last_update_time_;
  mutable typename bricks::time::EPOCH_MILLISECONDS time_to_be_ready_to_process_;
  const DistributionParams params_;
  std::string persistence_filename_;
  std::mt19937 rng_;
  std::exponential_distribution<double> distribution_;
};

}  // namespace strategy
}  // namespace fsq

#endif  // FSQ_EXPONENTIAL_RETRY_STRATEGY_H

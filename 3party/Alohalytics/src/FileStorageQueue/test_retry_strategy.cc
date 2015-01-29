// Tests for FSQ's retry strategy.
//
// The implementation of retry logic uses condition_variable::wait_for(),
// thus real delays on real clock are used for this test.
//
// The above makes the test non-deterministic in theory.
// In practice, the thresholds are liberal enough for it to pass.

#include <atomic>

#include "fsq.h"
#include "exponential_retry_strategy.h"

#include "../Bricks/dflags/dflags.h"

#include "../Bricks/file/file.h"
#include "../Bricks/strings/printf.h"

#include "../Bricks/3party/gtest/gtest.h"
#include "../Bricks/3party/gtest/gtest-main-with-dflags.h"

using std::string;
using std::atomic_size_t;

using bricks::time::EPOCH_MILLISECONDS;

DEFINE_string(tmpdir, "build/", "Directory to create temporary files in.");

DEFINE_int32(n, 200, "Number of FSQ-s to run. They will be run in parallel");
DEFINE_int32(number_of_failures, 1, "The first --number_of_failures processing attempts will fail.");

// Wait for 0.2s on average, with exponential distribution.
DEFINE_double(mean_ms, 200, "Retry exponential distribution parameter: Average time, in ms.");
DEFINE_double(min_ms, 0, "Retry exponential distribution parameter: Minimum time, in ms.");
DEFINE_double(max_ms, 5000, "Retry exponential distribution parameter: Maximum time, in ms.");

// At least 25% of delays should be under 0.1s and at least 25% should be over 0.25s.
DEFINE_double(p25_max, 100, "Maximum allowed value for 25-th percentile latency, in ms.");
DEFINE_double(p75_min, 250, "Minimum allowed value for 25-th percentile latency, in ms.");

DEFINE_bool(verbose, true, "Set to false to supress verbose output.");

#if 0
// TODO(dkorolev): Move to Bricks.
inline void SafeDebugOutput(const std::string& message) {
  if (FLAGS_verbose) {
    static std::mutex mutex;
    std::lock_guard<std::mutex> lock(mutex);
    std::cerr << message << std::endl;
  }
}
#endif

// LatencyMeasuringProcessor measures the time it took for the message to get processed.
struct LatencyMeasuringProcessor final {
  LatencyMeasuringProcessor(size_t index, atomic_size_t& counter)
      : index_(index), counter_(counter), failures_remaining_(FLAGS_number_of_failures), processed_(false) {}

  fsq::FileProcessingResult OnFileReady(const fsq::FileInfo<EPOCH_MILLISECONDS>&, EPOCH_MILLISECONDS now) {
    if (failures_remaining_ > 0) {
      --failures_remaining_;
      ++counter_;  // Important to only increment `counter_` after `processed_` is set, hence it's inside.
      return fsq::FileProcessingResult::FailureNeedRetry;
    } else {
      assert(failures_remaining_ == 0);
      assert(!processed_);
      processed_ = true;
      message_processed_timestamp_ = now;
      ++counter_;  // Important to only increment `counter_` after `processed_` is set, hence it's inside.
      return fsq::FileProcessingResult::Success;
    }
  }

  size_t index_;
  atomic_size_t& counter_;
  int failures_remaining_;
  bool processed_ = false;
  EPOCH_MILLISECONDS message_push_timestamp_;
  EPOCH_MILLISECONDS message_processed_timestamp_;
};

typedef fsq::Config<LatencyMeasuringProcessor> TestConfig;

template <typename T>
double Percentile(double p, const std::vector<T>& x) {
  assert(!x.empty());
  assert(p >= 0 && p <= 1);
  const double index = 1.0 * (x.size() - 1) * p;
  const size_t i0 = static_cast<size_t>(index);
  const size_t i1 = i0 + 1;
  const double w1 = index - i0;
  const double w0 = 1.0 - w1;
  double result = w0 * x[i0];
  if (i1 < x.size()) {
    result += w1 * x[i1];
  }
  return result;
}

TEST(FileSystemQueueLatenciesTest, LatencyPercentiles) {
  struct Worker final {
    typedef fsq::strategy::ExponentialDelayRetryStrategy<bricks::FileSystem> ExpRetry;
    Worker(int index, std::atomic_size_t& counter)
        : directory_name_(GenDirNameAndCreateDir(index)),
          index_(index),
          counter_(counter),
          processor_(index, counter_),
          fsq_(processor_,
               directory_name_,
               fsq::strategy::UseEpochMilliseconds(),
               bricks::FileSystem(),
               ExpRetry(bricks::FileSystem(),
                        ExpRetry::DistributionParams(FLAGS_mean_ms, FLAGS_min_ms, FLAGS_max_ms))) {}

    ~Worker() {
      // TODO(dkorolev): Remove created file(s) and the directory.
    }

    void InvokePushMessage() {
      processor_.message_push_timestamp_ = bricks::time::Now();
      fsq_.PushMessage("foo");
    }

    void InvokeFinalizeCurrentFile() { fsq_.FinalizeCurrentFile(); }

    uint64_t ActualProcessingLatencyInMS() const {
      assert(processor_.processed_);
      return static_cast<uint64_t>(processor_.message_processed_timestamp_ -
                                   processor_.message_push_timestamp_);
    }

    static std::string GenDirName(int index) {
      return bricks::FileSystem::JoinPath(FLAGS_tmpdir, bricks::strings::Printf("%05d", index));
    }

    static std::string GenDirNameAndCreateDir(int index) {
      std::string directory_name = GenDirName(index);
      bricks::FileSystem::CreateDirectory(directory_name);
      return directory_name;
    }

    std::string directory_name_;
    size_t index_;
    std::atomic_size_t& counter_;
    LatencyMeasuringProcessor processor_;
    fsq::FSQ<TestConfig> fsq_;
  };

  // Cleanup first.
  const size_t N = static_cast<size_t>(FLAGS_n);
  for (size_t i = 0; i < N; ++i) {
    struct DummyProcessor final {
      DummyProcessor() {}
      fsq::FileProcessingResult OnFileReady(const fsq::FileInfo<EPOCH_MILLISECONDS>&, EPOCH_MILLISECONDS) {
        return fsq::FileProcessingResult::Success;
      }
    };
    DummyProcessor processor;
    fsq::FSQ<fsq::Config<DummyProcessor>>(processor, Worker::GenDirName(i + 1)).ShutdownAndRemoveAllFSQFiles();
  }

  // Now, run the test.
  std::atomic_size_t counter(0);
  std::vector<std::unique_ptr<Worker>> workers;
  for (size_t i = 0; i < N; ++i) {
    workers.emplace_back(new Worker(i + 1, counter));
  }

  for (auto& it : workers) {
    it->InvokePushMessage();
  }

  for (auto& it : workers) {
    it->InvokeFinalizeCurrentFile();
  }

  while (counter != N * (1 + FLAGS_number_of_failures)) {
    ;  // Spin lock;
  }

  std::vector<uint64_t> latencies(N);
  for (size_t i = 0; i < N; ++i) {
    latencies[i] = workers[i]->ActualProcessingLatencyInMS();
  }
  std::sort(latencies.begin(), latencies.end());
  if (FLAGS_verbose) {
    std::cerr << "Latencies, ms:";
    for (size_t i = 0; i < N; ++i) {
      std::cerr << ' ' << latencies[i];
    }
    std::cerr << std::endl;
  }
  const double latency_p25_ms = Percentile(0.25, latencies);
  const double latency_p75_ms = Percentile(0.75, latencies);
  if (FLAGS_verbose) {
    std::cerr << "Latency average:          " << std::accumulate(latencies.begin(), latencies.end(), 0.0) / N
              << " ms\n";
    std::cerr << "Latency 25-th percentile: " << latency_p25_ms << " ms\n";
    std::cerr << "Latency 75-th percentile: " << latency_p75_ms << " ms\n";
  }

  EXPECT_LE(latency_p25_ms, FLAGS_p25_max);
  EXPECT_GE(latency_p75_ms, FLAGS_p75_min);
}

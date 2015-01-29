// TODO(dkorolev): Add a more purge test(s), code coverage should show which.

#include <atomic>

#include "fsq.h"

#include "../Bricks/file/file.h"

#include "../Bricks/3party/gtest/gtest.h"
#include "../Bricks/3party/gtest/gtest-main.h"

using std::string;
using std::atomic_size_t;

const char* const kTestDir = "build/";

// TestOutputFilesProcessor collects the output of finalized files.
struct TestOutputFilesProcessor {
  TestOutputFilesProcessor() : finalized_count(0) {}

  fsq::FileProcessingResult OnFileReady(const fsq::FileInfo<uint64_t>& file_info, uint64_t now) {
    if (mimic_unavailable_) {
      return fsq::FileProcessingResult::Unavailable;
    } else if (mimic_need_retry_) {
      return fsq::FileProcessingResult::FailureNeedRetry;
    } else {
      if (!finalized_count) {
        contents = bricks::ReadFileAsString(file_info.full_path_name);
        filenames = file_info.name;
      } else {
        contents = contents + "FILE SEPARATOR\n" + bricks::ReadFileAsString(file_info.full_path_name);
        filenames = filenames + "|" + file_info.name;
      }
      timestamp = now;
      ++finalized_count;
      return fsq::FileProcessingResult::Success;
    }
  }

  void ClearStats() {
    finalized_count = 0;
    filenames = "";
    contents = "";
    timestamp = 0;
  }

  void SetMimicUnavailable(bool mimic_unavailable = true) { mimic_unavailable_ = mimic_unavailable; }

  void SetMimicNeedRetry(bool mimic_need_retry = true) { mimic_need_retry_ = mimic_need_retry; }

  atomic_size_t finalized_count;
  string filenames = "";
  string contents = "";
  uint64_t timestamp = 0;

  bool mimic_unavailable_ = false;
  bool mimic_need_retry_ = false;
};

struct MockTime {
  typedef uint64_t T_TIMESTAMP;
  typedef int64_t T_TIME_SPAN;
  uint64_t now = 0;
  T_TIMESTAMP Now() const { return now; }
};

struct MockConfig : fsq::Config<TestOutputFilesProcessor> {
  // Mock time.
  typedef MockTime T_TIME_MANAGER;
  // Append using newlines.
  typedef fsq::strategy::AppendToFileWithSeparator T_FILE_APPEND_STRATEGY;
  // No backlog: 20 bytes 10 seconds old files max, with backlog: 100 bytes 60 seconds old files max.
  typedef fsq::strategy::SimpleFinalizationStrategy<MockTime::T_TIMESTAMP,
                                                    MockTime::T_TIME_SPAN,
                                                    20,
                                                    MockTime::T_TIME_SPAN(10 * 1000),
                                                    100,
                                                    MockTime::T_TIME_SPAN(60 * 1000)> T_FINALIZE_STRATEGY;
  // Purge after 50 bytes total or after 3 files.
  typedef fsq::strategy::SimplePurgeStrategy<50, 3> T_PURGE_STRATEGY;

  // Non-static initialization.
  template <typename T_FSQ_INSTANCE>
  static void Initialize(T_FSQ_INSTANCE& instance) {
    instance.SetSeparator("\n");
  }
};

struct NoResumeMockConfig : MockConfig {
  struct T_FILE_RESUME_STRATEGY {
    inline static bool ShouldResume() { return false; }
  };
};

typedef fsq::FSQ<MockConfig> FSQ;
typedef fsq::FSQ<NoResumeMockConfig> NoResumeFSQ;

static void CleanupOldFiles() {
  // Initialize a temporary FSQ to remove previously created files for the tests that need it.
  TestOutputFilesProcessor processor;
  FSQ(processor, kTestDir).ShutdownAndRemoveAllFSQFiles();
}

// Observe messages being processed as they exceed 20 bytes of size.
TEST(FileSystemQueueTest, FinalizedBySize) {
  CleanupOldFiles();

  TestOutputFilesProcessor processor;
  MockTime mock_wall_time;
  FSQ fsq(processor, kTestDir, mock_wall_time);

  // Confirm the queue is empty.
  EXPECT_EQ(0ull, fsq.GetQueueStatus().appended_file_size);
  EXPECT_EQ(0u, fsq.GetQueueStatus().finalized.queue.size());
  EXPECT_EQ(0ul, fsq.GetQueueStatus().finalized.total_size);

  // Add a few entries.
  mock_wall_time.now = 101;
  fsq.PushMessage("this is");
  mock_wall_time.now = 102;
  fsq.PushMessage("a test");
  mock_wall_time.now = 103;

  // Confirm the queue is still empty.
  EXPECT_EQ(15ull, fsq.GetQueueStatus().appended_file_size);  // 15 == strlen("this is\na test\n").
  EXPECT_EQ(0u, fsq.GetQueueStatus().finalized.queue.size());
  EXPECT_EQ(0ul, fsq.GetQueueStatus().finalized.total_size);
  EXPECT_EQ(0u, processor.finalized_count);

  // Add another message, under 20 bytes itself, that would make the current file exceed 20 bytes.
  fsq.PushMessage("process now");
  while (processor.finalized_count != 1) {
    ;  // Spin lock.
  }

  EXPECT_EQ(1u, processor.finalized_count);
  EXPECT_EQ("finalized-00000000000000000101.bin", processor.filenames);
  EXPECT_EQ("this is\na test\n", processor.contents);
  EXPECT_EQ(103ull, processor.timestamp);

  // Since the new message made it to the next file, finalize it and confirm the message is there as well.
  fsq.ForceProcessing();
  while (processor.finalized_count != 2) {
    ;  // Spin lock.
  }

  EXPECT_EQ(2u, processor.finalized_count);
  EXPECT_EQ("finalized-00000000000000000101.bin|finalized-00000000000000000103.bin", processor.filenames);
  EXPECT_EQ("this is\na test\nFILE SEPARATOR\nprocess now\n", processor.contents);
  EXPECT_EQ(103ull, processor.timestamp);
}

// Observe messages being processed as they get older than 10 seconds.
TEST(FileSystemQueueTest, FinalizedByAge) {
  CleanupOldFiles();

  TestOutputFilesProcessor processor;
  MockTime mock_wall_time;
  FSQ fsq(processor, kTestDir, mock_wall_time);

  // Confirm the queue is empty.
  EXPECT_EQ(0ull, fsq.GetQueueStatus().appended_file_size);
  EXPECT_EQ(0u, fsq.GetQueueStatus().finalized.queue.size());
  EXPECT_EQ(0ul, fsq.GetQueueStatus().finalized.total_size);

  // Add a few entries.
  mock_wall_time.now = 10000;
  fsq.PushMessage("this too");
  mock_wall_time.now = 10001;
  fsq.PushMessage("shall");

  // Confirm the queue is still empty.
  EXPECT_EQ(15ull, fsq.GetQueueStatus().appended_file_size);  // 15 == strlen("this is\na test\n").
  EXPECT_EQ(0u, fsq.GetQueueStatus().finalized.queue.size());
  EXPECT_EQ(0ul, fsq.GetQueueStatus().finalized.total_size);
  EXPECT_EQ(0u, processor.finalized_count);

  // Add another message and make the current file span an interval of more than 10 seconds.
  mock_wall_time.now = 21000;
  fsq.PushMessage("pass");

  while (processor.finalized_count != 1) {
    ;  // Spin lock.
  }

  EXPECT_EQ(1u, processor.finalized_count);
  EXPECT_EQ("finalized-00000000000000010000.bin", processor.filenames);
  EXPECT_EQ("this too\nshall\n", processor.contents);
  EXPECT_EQ(21000ull, processor.timestamp);

  // Since the new message made it to the next file, finalize it and confirm the message is there as well.
  fsq.ForceProcessing();
  while (processor.finalized_count != 2) {
    ;  // Spin lock.
  }

  EXPECT_EQ(2u, processor.finalized_count);
  EXPECT_EQ("finalized-00000000000000010000.bin|finalized-00000000000000021000.bin", processor.filenames);
  EXPECT_EQ("this too\nshall\nFILE SEPARATOR\npass\n", processor.contents);
  EXPECT_EQ(21000ull, processor.timestamp);
}

// Pushes a few messages and force their processing.
TEST(FileSystemQueueTest, ForceProcessing) {
  CleanupOldFiles();

  TestOutputFilesProcessor processor;
  MockTime mock_wall_time;
  FSQ fsq(processor, kTestDir, mock_wall_time);

  // Confirm the queue is empty.
  EXPECT_EQ(0ull, fsq.GetQueueStatus().appended_file_size);
  EXPECT_EQ(0u, fsq.GetQueueStatus().finalized.queue.size());
  EXPECT_EQ(0ul, fsq.GetQueueStatus().finalized.total_size);

  // Add a few entries.
  mock_wall_time.now = 1001;
  fsq.PushMessage("foo");
  mock_wall_time.now = 1002;
  fsq.PushMessage("bar");
  mock_wall_time.now = 1003;
  fsq.PushMessage("baz");

  // Confirm the queue is empty.
  EXPECT_EQ(12ull, fsq.GetQueueStatus().appended_file_size);  // Three messages of (3 + '\n') bytes each.
  EXPECT_EQ(0u, fsq.GetQueueStatus().finalized.queue.size());
  EXPECT_EQ(0ul, fsq.GetQueueStatus().finalized.total_size);

  // Force entries processing to have three freshly added ones reach our TestOutputFilesProcessor.
  fsq.ForceProcessing();
  while (!processor.finalized_count) {
    ;  // Spin lock.
  }

  EXPECT_EQ(1u, processor.finalized_count);
  EXPECT_EQ("finalized-00000000000000001001.bin", processor.filenames);
  EXPECT_EQ("foo\nbar\nbaz\n", processor.contents);
  EXPECT_EQ(1003ull, processor.timestamp);
}

// Confirm the existing file is resumed.
TEST(FileSystemQueueTest, ResumesExistingFile) {
  CleanupOldFiles();

  TestOutputFilesProcessor processor;
  MockTime mock_wall_time;

  bricks::WriteStringToFile(bricks::FileSystem::JoinPath(kTestDir, "current-00000000000000000001.bin"),
                            "meh\n");

  FSQ fsq(processor, kTestDir, mock_wall_time);

  mock_wall_time.now = 1;
  fsq.PushMessage("wow");

  fsq.ForceProcessing();
  while (!processor.finalized_count) {
    ;  // Spin lock.
  }

  EXPECT_EQ(1u, processor.finalized_count);
  EXPECT_EQ("finalized-00000000000000000001.bin", processor.filenames);
  EXPECT_EQ("meh\nwow\n", processor.contents);
}

// Confirm only one existing file is resumed, the rest are finalized.
TEST(FileSystemQueueTest, ResumesOnlyExistingFileAndFinalizesTheRest) {
  CleanupOldFiles();

  TestOutputFilesProcessor processor;
  MockTime mock_wall_time;

  bricks::WriteStringToFile(bricks::FileSystem::JoinPath(kTestDir, "current-00000000000000000001.bin"),
                            "one\n");
  bricks::WriteStringToFile(bricks::FileSystem::JoinPath(kTestDir, "current-00000000000000000002.bin"),
                            "two\n");
  bricks::WriteStringToFile(bricks::FileSystem::JoinPath(kTestDir, "current-00000000000000000003.bin"),
                            "three\n");

  FSQ fsq(processor, kTestDir, mock_wall_time);

  while (processor.finalized_count != 2) {
    ;  // Spin lock.
  }

  EXPECT_EQ(2u, processor.finalized_count);
  EXPECT_EQ("finalized-00000000000000000001.bin|finalized-00000000000000000002.bin", processor.filenames);
  EXPECT_EQ("one\nFILE SEPARATOR\ntwo\n", processor.contents);
  processor.ClearStats();

  mock_wall_time.now = 4;
  fsq.PushMessage("four");

  fsq.ForceProcessing();
  while (processor.finalized_count != 1) {
    ;  // Spin lock.
  }

  EXPECT_EQ(1u, processor.finalized_count);
  EXPECT_EQ("finalized-00000000000000000003.bin", processor.filenames);
  EXPECT_EQ("three\nfour\n", processor.contents);
}

// Confirm the existing file is not resumed if the strategy dictates so.
TEST(FileSystemQueueTest, ResumeCanBeTurnedOff) {
  CleanupOldFiles();

  TestOutputFilesProcessor processor;
  MockTime mock_wall_time;

  bricks::WriteStringToFile(bricks::FileSystem::JoinPath(kTestDir, "current-00000000000000000000.bin"),
                            "meh\n");

  NoResumeFSQ fsq(processor, kTestDir, mock_wall_time);

  while (processor.finalized_count != 1) {
    ;  // Spin lock.
  }

  EXPECT_EQ("finalized-00000000000000000000.bin", processor.filenames);
  EXPECT_EQ("meh\n", processor.contents);

  mock_wall_time.now = 1;
  fsq.PushMessage("wow");

  fsq.ForceProcessing();
  while (processor.finalized_count != 2) {
    ;  // Spin lock.
  }

  EXPECT_EQ(2u, processor.finalized_count);
  EXPECT_EQ("finalized-00000000000000000000.bin|finalized-00000000000000000001.bin", processor.filenames);
  EXPECT_EQ("meh\nFILE SEPARATOR\nwow\n", processor.contents);
}

// Purges the oldest files so that there are at most three in the queue of the finalized ones.
TEST(FileSystemQueueTest, PurgesByNumberOfFiles) {
  CleanupOldFiles();

  TestOutputFilesProcessor processor;
  processor.SetMimicUnavailable();
  MockTime mock_wall_time;
  FSQ fsq(processor, kTestDir, mock_wall_time);

  // Add a few files.
  mock_wall_time.now = 100001;
  fsq.PushMessage("one");
  fsq.FinalizeCurrentFile();
  mock_wall_time.now = 100002;
  fsq.PushMessage("two");
  fsq.FinalizeCurrentFile();
  mock_wall_time.now = 100003;
  fsq.PushMessage("three");
  fsq.FinalizeCurrentFile();

  // Confirm the queue contains three files.
  EXPECT_EQ(3u, fsq.GetQueueStatus().finalized.queue.size());
  EXPECT_EQ(14ul, fsq.GetQueueStatus().finalized.total_size);  // strlen("one\ntwo\nthree\n").
  EXPECT_EQ("finalized-00000000000000100001.bin", fsq.GetQueueStatus().finalized.queue.front().name);
  EXPECT_EQ("finalized-00000000000000100003.bin", fsq.GetQueueStatus().finalized.queue.back().name);

  // Add the fourth file.
  mock_wall_time.now = 100004;
  fsq.PushMessage("four");
  fsq.FinalizeCurrentFile();

  // Confirm the first one got deleted.
  EXPECT_EQ(3u, fsq.GetQueueStatus().finalized.queue.size());
  EXPECT_EQ(15ul, fsq.GetQueueStatus().finalized.total_size);  // strlen("two\nthree\nfour\n").
  EXPECT_EQ("finalized-00000000000000100002.bin", fsq.GetQueueStatus().finalized.queue.front().name);
  EXPECT_EQ("finalized-00000000000000100004.bin", fsq.GetQueueStatus().finalized.queue.back().name);
}

// Purges the oldest files so that the total size of the queue never exceeds 20 bytes.
TEST(FileSystemQueueTest, PurgesByTotalSize) {
  CleanupOldFiles();

  TestOutputFilesProcessor processor;
  processor.SetMimicUnavailable();
  MockTime mock_wall_time;
  FSQ fsq(processor, kTestDir, mock_wall_time);

  mock_wall_time.now = 100001;
  fsq.PushMessage("one");
  mock_wall_time.now = 100002;
  fsq.PushMessage("two");
  fsq.FinalizeCurrentFile();
  mock_wall_time.now = 100003;
  fsq.PushMessage("three");
  mock_wall_time.now = 100004;
  fsq.PushMessage("four");
  fsq.FinalizeCurrentFile();

  // Confirm the queue contains two files.
  EXPECT_EQ(2u, fsq.GetQueueStatus().finalized.queue.size());
  EXPECT_EQ(19ul, fsq.GetQueueStatus().finalized.total_size);  // strlen("one\ntwo\nthree\nfour\n").
  EXPECT_EQ("finalized-00000000000000100001.bin", fsq.GetQueueStatus().finalized.queue.front().name);
  EXPECT_EQ("finalized-00000000000000100003.bin", fsq.GetQueueStatus().finalized.queue.back().name);

  // Add another file of the size that would force the oldest one to get purged.
  mock_wall_time.now = 100010;
  fsq.PushMessage("very, very, very, very long message");
  fsq.FinalizeCurrentFile();

  // Confirm the oldest file got deleted.
  EXPECT_EQ(2u, fsq.GetQueueStatus().finalized.queue.size());
  EXPECT_EQ(47l, fsq.GetQueueStatus().finalized.total_size);
  EXPECT_EQ("finalized-00000000000000100003.bin", fsq.GetQueueStatus().finalized.queue.front().name);
  EXPECT_EQ("finalized-00000000000000100010.bin", fsq.GetQueueStatus().finalized.queue.back().name);
}

// Persists retry delay to the file.
TEST(FileSystemQueueTest, SavesRetryDelayToFile) {
  const std::string state_file_name = std::move(bricks::FileSystem::JoinPath(kTestDir, "state"));

  CleanupOldFiles();
  bricks::RemoveFile(state_file_name, bricks::RemoveFileParameters::Silent);

  const uint64_t t1 = static_cast<uint64_t>(bricks::time::Now());

  TestOutputFilesProcessor processor;
  MockTime mock_wall_time;
  typedef fsq::strategy::ExponentialDelayRetryStrategy<bricks::FileSystem> ExpRetry;
  // Wait between 1 second and 2 seconds before retrying.
  FSQ fsq(processor,
          kTestDir,
          mock_wall_time,
          bricks::FileSystem(),
          ExpRetry(bricks::FileSystem(), ExpRetry::DistributionParams(1500, 1000, 2000)));

  // Attach to file, this will force the creation of the file.
  fsq.AttachToFile(state_file_name);

  const uint64_t t2 = static_cast<uint64_t>(bricks::time::Now());

  // At start, with no file to resume, update time should be equal to next processing ready time,
  // and they should both be between `t1` and `t2` retrieved above.
  std::string contents1 = bricks::ReadFileAsString(state_file_name);
  ASSERT_EQ(contents1.length(), 41);  // 20 + 1 + 20.
  std::istringstream is1(contents1);
  uint64_t a1, b1;
  is1 >> a1 >> b1;
  EXPECT_GE(a1, t1);
  EXPECT_LE(a1, t2);
  EXPECT_GE(b1, t1);
  EXPECT_LE(b1, t2);
  EXPECT_EQ(a1, b1);

  // Now, after one failure, update time whould be between `t4` and `t4`,
  // and next processing time should be between one and two seconds into the future.
  const uint64_t t3 = static_cast<uint64_t>(bricks::time::Now());
  fsq.PushMessage("blah");
  processor.SetMimicNeedRetry();
  fsq.ForceProcessing();
  const uint64_t t4 = static_cast<uint64_t>(bricks::time::Now());

  // Give FSQ some time to apply retry policy and to update the state file.
  const uint64_t SAFETY_INTERVAL = 50;  // Wait for 50ms to ensure the file is updated.
  // TODO(dkorolev): This makes the test flaky, but should be fine for Alex to go ahead and push it.

  std::this_thread::sleep_for(std::chrono::milliseconds(SAFETY_INTERVAL));

  std::string contents2 = bricks::ReadFileAsString(state_file_name);
  ASSERT_EQ(contents2.length(), 41);  // 20 + 1 + 20.
  std::istringstream is2(contents2);
  uint64_t a2, b2;
  is2 >> a2 >> b2;
  EXPECT_GE(a2, t3);
  EXPECT_LE(a2, t4 + SAFETY_INTERVAL);
  EXPECT_GE(b2, a2 + 1000);
  EXPECT_LE(b2, a2 + 2000);
}

// Reads retry delay from file.
TEST(FileSystemQueueTest, ReadsRetryDelayFromFile) {
  const std::string state_file_name = std::move(bricks::FileSystem::JoinPath(kTestDir, "state"));

  CleanupOldFiles();

  // Legitimately configure FSQ to wait for 5 seconds from now before further processing takes place.
  const uint64_t t1 = static_cast<uint64_t>(bricks::time::Now());
  using bricks::strings::PackToString;
  bricks::WriteStringToFile(state_file_name.c_str(), PackToString(t1) + ' ' + PackToString(t1 + 5000));

  TestOutputFilesProcessor processor;
  MockTime mock_wall_time;
  typedef fsq::strategy::ExponentialDelayRetryStrategy<bricks::FileSystem> ExpRetry;
  FSQ fsq(processor,
          kTestDir,
          mock_wall_time,
          bricks::FileSystem(),
          ExpRetry(bricks::FileSystem(), ExpRetry::DistributionParams(1500, 1000, 2000)));
  fsq.AttachToFile(state_file_name);

  const uint64_t t2 = static_cast<uint64_t>(bricks::time::Now());

  // Should wait for 5 more seconds, perhaps without a few milliseconds it took to read the file, etc.
  bricks::time::MILLISECONDS_INTERVAL d;
  ASSERT_TRUE(fsq.ShouldWait(&d));
  EXPECT_GE(t2, t1);
  EXPECT_LE(t2 - t1, 50);
  EXPECT_GE(static_cast<uint64_t>(d), 4950);
  EXPECT_LE(static_cast<uint64_t>(d), 5000);
}

// Ignored retry delay if it was set from the future.
TEST(FileSystemQueueTest, IgnoredRetryDelaySetFromTheFuture) {
  const std::string state_file_name = std::move(bricks::FileSystem::JoinPath(kTestDir, "state"));

  CleanupOldFiles();

  // Incorrectly configure FSQ to start 5 seconds from now, set at 0.5 seconds into the future.
  const uint64_t t = static_cast<uint64_t>(bricks::time::Now());
  using bricks::strings::PackToString;
  bricks::WriteStringToFile(state_file_name.c_str(), PackToString(t + 500) + ' ' + PackToString(t + 5000));

  TestOutputFilesProcessor processor;
  MockTime mock_wall_time;
  typedef fsq::strategy::ExponentialDelayRetryStrategy<bricks::FileSystem> ExpRetry;
  FSQ fsq(processor,
          kTestDir,
          mock_wall_time,
          bricks::FileSystem(),
          ExpRetry(bricks::FileSystem(), ExpRetry::DistributionParams(1500, 1000, 2000)));
  fsq.AttachToFile(state_file_name);

  bricks::time::MILLISECONDS_INTERVAL interval;
  ASSERT_FALSE(fsq.ShouldWait(&interval));
}

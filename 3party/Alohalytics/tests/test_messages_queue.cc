/*******************************************************************************
The MIT License (MIT)

Copyright (c) 2015 Alexander Zolotarev <me@alex.bio> from Minsk, Belarus

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*******************************************************************************/

#include "gtest/gtest.h"

#include "generate_temporary_file_name.h"
#include "../src/file_manager.h"
#include "../src/messages_queue.h"

#include <random>
#include <vector>

using alohalytics::FileManager;
using alohalytics::ScopedRemoveFile;

using alohalytics::HundredKilobytesFileQueue;
using alohalytics::ProcessingResult;

bool EndsWith(const std::string & str, const std::string & suffix) {
  const std::string::size_type str_size = str.size(), suffix_size = suffix.size();
  return str_size >= suffix_size && std::equal(suffix.begin(), suffix.end(), str.end() - suffix_size);
}

TEST(MessagesQueue, EndsWith) {
  EXPECT_TRUE(EndsWith("", ""));
  EXPECT_TRUE(EndsWith("Hello, World!", " World!"));
  EXPECT_TRUE(EndsWith("Hello", "Hello"));
  EXPECT_FALSE(EndsWith("Hello, World!", " World! "));
  EXPECT_FALSE(EndsWith("Hell", "Hello"));
  EXPECT_FALSE(EndsWith("ello", "Hello"));
}

// Removes all MessagesQueue's files in the directory.
void CleanUpQueueFiles(const std::string & directory) {
  FileManager::ForEachFileInDir(directory, [](const std::string & file) {
    using namespace alohalytics;
    if (EndsWith(file, alohalytics::kArchivedFilesExtension) || EndsWith(file, alohalytics::kCurrentFileName)) {
      std::remove(file.c_str());
    }
    return true;
  });
}

// For debug logging.
std::ostream & operator<<(std::ostream & os, ProcessingResult result) {
  switch (result) {
    case ProcessingResult::ENothingToProcess:
      os << "ENothingToProcess";
      break;
    case ProcessingResult::EProcessedSuccessfully:
      os << "EProcessedSuccessfully";
      break;
    case ProcessingResult::EProcessingError:
      os << "EProcessingError";
      break;
  }
  return os;
}

// Helper class to avoid data races in unit tests.
struct FinishTask {
  FinishTask() : triggered_(false), result_(ProcessingResult::ENothingToProcess) {}

  void operator()(ProcessingResult result) {
    std::lock_guard<std::mutex> lock(mu_);
    triggered_ = true;
    result_ = result;
    cv_.notify_one();
  }

  ProcessingResult get() {
    std::unique_lock<std::mutex> lock(mu_);
    cv_.wait(lock, [this]() { return triggered_; });
    return result_;
  }

  std::mutex mu_;
  std::condition_variable cv_;
  bool triggered_;
  ProcessingResult result_;
};

static const std::string kTestMessage = "Test Message";
static const std::string kTestWorkerMessage = "I am worker thread!";

// Executed on the WorkingThread.
static void FinishedCallback(ProcessingResult result, FinishTask & finish_task) {
  // Pass callback result to the future.
  finish_task(result);
}

TEST(MessagesQueue, InMemory_Empty) {
  bool processor_was_called = false;
  HundredKilobytesFileQueue q;
  FinishTask finish_task;
  q.ProcessArchivedFiles([&processor_was_called](bool, const std::string &) {
    processor_was_called = true;  // This code should not be executed.
    return false;
  }, std::bind(&FinishedCallback, std::placeholders::_1, std::ref(finish_task)));
  EXPECT_EQ(ProcessingResult::ENothingToProcess, finish_task.get());
  EXPECT_FALSE(processor_was_called);
}

TEST(MessagesQueue, InMemory_SuccessfulProcessing) {
  HundredKilobytesFileQueue q;
  q.PushMessage(kTestMessage);
  std::thread worker([&q]() { q.PushMessage(kTestWorkerMessage); });
  worker.join();
  bool processor_was_called = false;
  FinishTask finish_task;
  q.ProcessArchivedFiles([&processor_was_called](bool is_file, const std::string & messages) {
    EXPECT_FALSE(is_file);
    EXPECT_EQ(messages, kTestMessage + kTestWorkerMessage);
    processor_was_called = true;
    return true;
  }, std::bind(&FinishedCallback, std::placeholders::_1, std::ref(finish_task)));
  EXPECT_EQ(ProcessingResult::EProcessedSuccessfully, finish_task.get());
  EXPECT_TRUE(processor_was_called);
}

TEST(MessagesQueue, InMemory_FailedProcessing) {
  HundredKilobytesFileQueue q;
  q.PushMessage(kTestMessage);
  bool processor_was_called = false;
  FinishTask finish_task;
  q.ProcessArchivedFiles([&processor_was_called](bool is_file, const std::string & messages) {
    EXPECT_FALSE(is_file);
    EXPECT_EQ(messages, kTestMessage);
    processor_was_called = true;
    return false;
  }, std::bind(&FinishedCallback, std::placeholders::_1, std::ref(finish_task)));
  EXPECT_EQ(ProcessingResult::EProcessingError, finish_task.get());
  EXPECT_TRUE(processor_was_called);
}

TEST(MessagesQueue, SwitchFromInMemoryToFile_and_OfflineEmulation) {
  const std::string tmpdir = FileManager::GetDirectoryFromFilePath(GenerateTemporaryFileName());
  CleanUpQueueFiles(tmpdir);
  const ScopedRemoveFile remover(tmpdir + alohalytics::kCurrentFileName);
  std::string archived_file, second_archived_file;
  {
    {
      HundredKilobytesFileQueue q;
      q.PushMessage(kTestMessage);    // This one goes into the memory storage.
      q.SetStorageDirectory(tmpdir);  // Here message shoud move from memory into the file.
      std::thread worker([&q]() { q.PushMessage(kTestWorkerMessage); });
      worker.join();
      // After calling queue's destructor, all messages should be gracefully stored in the file.
    }
    EXPECT_EQ(kTestMessage + kTestWorkerMessage, FileManager::ReadFileAsString(tmpdir + alohalytics::kCurrentFileName));

    bool processor_was_called = false;
    FinishTask finish_task;
    HundredKilobytesFileQueue q;
    q.SetStorageDirectory(tmpdir);
    q.ProcessArchivedFiles([&processor_was_called, &archived_file](bool is_file, const std::string & full_file_path) {
      EXPECT_TRUE(is_file);
      EXPECT_EQ(kTestMessage + kTestWorkerMessage, FileManager::ReadFileAsString(full_file_path));
      processor_was_called = true;
      archived_file = full_file_path;
      return false;  // Emulate network error.
    }, std::bind(&FinishedCallback, std::placeholders::_1, std::ref(finish_task)));
    EXPECT_EQ(ProcessingResult::EProcessingError, finish_task.get());
    EXPECT_TRUE(processor_was_called);
    // Current file should be empty as it was archived for processing.
    EXPECT_EQ("", FileManager::ReadFileAsString(tmpdir + alohalytics::kCurrentFileName));
    EXPECT_EQ(kTestMessage + kTestWorkerMessage, FileManager::ReadFileAsString(archived_file));
  }

  // Create second archive in the queue after ProcessArchivedFiles() call.
  HundredKilobytesFileQueue q;
  q.SetStorageDirectory(tmpdir);
  q.PushMessage(kTestMessage);
  {
    bool archive1_processed = false, archive2_processed = false;
    FinishTask finish_task;
    q.ProcessArchivedFiles([&](bool is_file, const std::string & full_file_path) {
      EXPECT_TRUE(is_file);
      if (full_file_path == archived_file) {
        EXPECT_EQ(kTestMessage + kTestWorkerMessage, FileManager::ReadFileAsString(full_file_path));
        archive1_processed = true;
      } else {
        EXPECT_EQ(kTestMessage, FileManager::ReadFileAsString(full_file_path));
        second_archived_file = full_file_path;
        archive2_processed = true;
      }
      return true;  // Archives should be deleted by queue after successful processing.
    }, std::bind(&FinishedCallback, std::placeholders::_1, std::ref(finish_task)));
    EXPECT_EQ(ProcessingResult::EProcessedSuccessfully, finish_task.get());
    EXPECT_TRUE(archive1_processed);
    EXPECT_TRUE(archive2_processed);
    EXPECT_THROW(FileManager::ReadFileAsString(archived_file), std::ios_base::failure);
    EXPECT_THROW(FileManager::ReadFileAsString(second_archived_file), std::ios_base::failure);
  }
}

TEST(MessagesQueue, CreateArchiveOnSizeLimitHit) {
  const std::string tmpdir = FileManager::GetDirectoryFromFilePath(GenerateTemporaryFileName());
  CleanUpQueueFiles(tmpdir);
  const ScopedRemoveFile remover(tmpdir + alohalytics::kCurrentFileName);
  HundredKilobytesFileQueue q;
  q.SetStorageDirectory(tmpdir);

  // Generate messages with total size enough for triggering archiving.
  std::ofstream::pos_type size = 0;
  auto const generator = [&q, &size](const std::string & message, std::ofstream::pos_type limit) {
    std::ofstream::pos_type generated_size = 0;
    while (generated_size < limit) {
      q.PushMessage(message);
      generated_size += message.size();
    }
    size += generated_size;
  };
  static const std::ofstream::pos_type number_of_bytes_to_generate =
      HundredKilobytesFileQueue::kMaxFileSizeInBytes / 2 + 100;
  std::thread worker([&generator]() { generator(kTestWorkerMessage, number_of_bytes_to_generate); });
  generator(kTestMessage, number_of_bytes_to_generate);
  worker.join();

  std::vector<std::ofstream::pos_type> file_sizes;
  FinishTask finish_task;
  q.ProcessArchivedFiles([&file_sizes](bool is_file, const std::string & full_file_path) {
    EXPECT_TRUE(is_file);
    file_sizes.push_back(FileManager::ReadFileAsString(full_file_path).size());
    return true;
  }, std::bind(&FinishedCallback, std::placeholders::_1, std::ref(finish_task)));
  EXPECT_EQ(ProcessingResult::EProcessedSuccessfully, finish_task.get());
  EXPECT_EQ(size_t(2), file_sizes.size());
  EXPECT_EQ(size, file_sizes[0] + file_sizes[1]);
  EXPECT_TRUE((file_sizes[0] > q.kMaxFileSizeInBytes) != (file_sizes[1] > q.kMaxFileSizeInBytes));
}

TEST(MessagesQueue, HighLoadAndIntegrity) {
  // TODO(AlexZ): This test can be improved by generating really a lot of data
  // so many archives will be created. But it will make everything much more complex now.
  const std::string tmpdir = FileManager::GetDirectoryFromFilePath(GenerateTemporaryFileName());
  CleanUpQueueFiles(tmpdir);
  const ScopedRemoveFile remover(tmpdir + alohalytics::kCurrentFileName);
  HundredKilobytesFileQueue q;
  const int kMaxThreads = 300;
  std::mt19937 gen(std::mt19937::default_seed);
  std::uniform_int_distribution<> dis('A', 'Z');
  auto const generator = [&q](char c) { q.PushMessage(std::string(static_cast<size_t>(c), c)); };
  std::vector<std::thread> threads;
  size_t total_size = 0;
  for (int i = 0; i < kMaxThreads; ++i) {
    char c = dis(gen);
    total_size += static_cast<size_t>(c);
    if (i == kMaxThreads / 2) {
      // At first, messages go into the in-memory queue. Then we initialize files storage.
      q.SetStorageDirectory(tmpdir);
    }
    std::thread worker([&generator, c]() { generator(c); });
    threads.push_back(std::move(worker));
  }
  EXPECT_TRUE(total_size > 0);
  for (auto & thread : threads) {
    thread.join();
  }
  FinishTask finish_task;
  q.ProcessArchivedFiles([&total_size](bool is_file, const std::string & full_file_path) {
    EXPECT_TRUE(is_file);
    const std::string data = FileManager::ReadFileAsString(full_file_path);
    EXPECT_EQ(total_size, data.size());
    // Integrity check.
    size_t beg = 0, end = 0;
    while (beg < data.size()) {
      end += data[beg];
      const size_t count = end - beg;
      EXPECT_EQ(std::string(static_cast<size_t>(data[beg]), count), data.substr(beg, count));
      beg += count;
    }
    total_size = 0;
    return true;
  }, std::bind(&FinishedCallback, std::placeholders::_1, std::ref(finish_task)));
  EXPECT_EQ(ProcessingResult::EProcessedSuccessfully, finish_task.get());
  EXPECT_EQ(size_t(0), total_size);  // Zero means that processor was called.
}

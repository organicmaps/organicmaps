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

#include "../src/file_manager.h"
#include "../src/messages_queue.h"

#include <algorithm>
#include <chrono>
#include <cstdlib>
#include <future>
#include <iostream>
#include <map>
#include <random>
#include <thread>
#include <vector>

#define TEST_EQUAL(x, y)                                                                                        \
  {                                                                                                             \
    auto vx = (x);                                                                                              \
    auto vy = (y);                                                                                              \
    if (vx != vy) {                                                                                             \
      std::cerr << __FILE__ << ':' << __FUNCTION__ << ':' << __LINE__ << " Test failed: " << #x << " != " << #y \
                << " (" << vx << " != " << vy << ")" << std::endl;                                              \
      std::exit(-1);                                                                                            \
    }                                                                                                           \
  }

#define TEST_EXCEPTION(ex, op)                                                                                   \
  {                                                                                                              \
    bool has_fired = false;                                                                                      \
    try {                                                                                                        \
      op;                                                                                                        \
    } catch (const std::exception & exc) {                                                                       \
      has_fired = true;                                                                                          \
      if (typeid(ex) != typeid(exc)) {                                                                           \
        std::cerr << __FILE__ << ':' << __FUNCTION__ << ':' << __LINE__ << " Test failed: " << typeid(ex).name() \
                  << " != " << typeid(exc).name() << std::endl;                                                  \
        std::exit(-1);                                                                                           \
      }                                                                                                          \
    }                                                                                                            \
    if (!has_fired) {                                                                                            \
      std::cerr << __FILE__ << ':' << __FUNCTION__ << ':' << __LINE__ << " Test failed: "                        \
                << "Exception " << typeid(ex).name() << "Was not thrown." << std::endl;                          \
      std::exit(-1);                                                                                             \
    }                                                                                                            \
  }

using alohalytics::FileManager;
using alohalytics::ScopedRemoveFile;

// Generates unique temporary file name or empty string on error.
static std::string GenerateTemporaryFileName() {
#ifdef _MSC_VER
  char tmp_file[L_tmpnam];
  if (0 == ::tmpnam_s(tmp_file, L_tmpnam)) {
    return tmp_file;
  }
#else
  char tmp_file[] = "/tmp/alohalytics_file_manager-XXXXXX";
  if (::mktemp(tmp_file)) {
    return tmp_file;
  }
#endif
  return std::string();
}

void Test_GetDirectoryFromFilePath() {
  const std::string s = std::string(1, FileManager::kDirectorySeparator);
  const std::string ns = (s == "/") ? "\\" : "/";
  TEST_EQUAL("", FileManager::GetDirectoryFromFilePath(""));
  TEST_EQUAL(".", FileManager::GetDirectoryFromFilePath("some_file_name.ext"));
  TEST_EQUAL(".", FileManager::GetDirectoryFromFilePath("evil" + ns + "file"));
  TEST_EQUAL("dir" + s, FileManager::GetDirectoryFromFilePath("dir" + s + "file"));
  TEST_EQUAL(s + "root" + s + "dir" + s, FileManager::GetDirectoryFromFilePath(s + "root" + s + "dir" + s + "file"));
  TEST_EQUAL(".", FileManager::GetDirectoryFromFilePath("dir" + ns + "file"));
  TEST_EQUAL("C:" + s + "root" + s + "dir" + s,
             FileManager::GetDirectoryFromFilePath("C:" + s + "root" + s + "dir" + s + "file.ext"));
  TEST_EQUAL(s + "tmp" + s, FileManager::GetDirectoryFromFilePath(s + "tmp" + s + "evil" + ns + "file"));
}

void Test_ScopedRemoveFile() {
  const std::string file = GenerateTemporaryFileName();
  {
    ScopedRemoveFile remover(file);
    TEST_EQUAL(true, FileManager::AppendStringToFile(file, file));
    TEST_EQUAL(file, FileManager::ReadFileAsString(file));
  }
  TEST_EXCEPTION(std::ios_base::failure, FileManager::ReadFileAsString(file));
}

void Test_CreateTemporaryFile() {
  const std::string file1 = GenerateTemporaryFileName();
  ScopedRemoveFile remover1(file1);
  TEST_EQUAL(true, FileManager::AppendStringToFile(file1, file1));
  TEST_EQUAL(file1, FileManager::ReadFileAsString(file1));
  const std::string file2 = GenerateTemporaryFileName();
  TEST_EQUAL(false, file1 == file2);
  ScopedRemoveFile remover2(file2);
  TEST_EQUAL(true, FileManager::AppendStringToFile(file2, file2));
  TEST_EQUAL(file2, FileManager::ReadFileAsString(file2));
  TEST_EQUAL(true, file1 != file2);
}

void Test_AppendStringToFile() {
  const std::string file = GenerateTemporaryFileName();
  ScopedRemoveFile remover(file);
  const std::string s1("First\0 String");
  TEST_EQUAL(true, FileManager::AppendStringToFile(s1, file));
  TEST_EQUAL(s1, FileManager::ReadFileAsString(file));
  const std::string s2("Second one.");
  TEST_EQUAL(true, FileManager::AppendStringToFile(s2, file));
  TEST_EQUAL(s1 + s2, FileManager::ReadFileAsString(file));

  TEST_EQUAL(false, FileManager::AppendStringToFile(file, ""));
}

void Test_ReadFileAsString() {
  const std::string file = GenerateTemporaryFileName();
  ScopedRemoveFile remover(file);
  TEST_EQUAL(true, FileManager::AppendStringToFile(file, file));
  TEST_EQUAL(file, FileManager::ReadFileAsString(file));
}

void Test_ForEachFileInDir() {
  {
    bool was_called_at_least_once = false;
    FileManager::ForEachFileInDir("", [&was_called_at_least_once](const std::string &) -> bool {
      was_called_at_least_once = true;
      return true;
    });
    TEST_EQUAL(false, was_called_at_least_once);
  }

  {
    std::vector<std::string> files, files_copy;
    std::vector<std::unique_ptr<ScopedRemoveFile>> removers;
    for (size_t i = 0; i < 5; ++i) {
      const std::string file = GenerateTemporaryFileName();
      files.push_back(file);
      removers.emplace_back(new ScopedRemoveFile(file));
      TEST_EQUAL(true, FileManager::AppendStringToFile(file, file));
    }
    files_copy = files;
    const std::string directory = FileManager::GetDirectoryFromFilePath(files[0]);
    TEST_EQUAL(false, directory.empty());
    FileManager::ForEachFileInDir(directory, [&files_copy](const std::string & path) -> bool {
      // Some random files can remain in the temporary directory.
      const auto found = std::find(files_copy.begin(), files_copy.end(), path);
      if (found != files_copy.end()) {
        TEST_EQUAL(path, FileManager::ReadFileAsString(path));
        files_copy.erase(found);
      }
      return true;
    });
    TEST_EQUAL(size_t(0), files_copy.size());

    // Test if ForEachFileInDir can be correctly interrupted in the middle.
    files_copy = files;
    FileManager::ForEachFileInDir(directory, [&files_copy](const std::string & path) -> bool {
      // Some random files can remain in the temporary directory.
      const auto found = std::find(files_copy.begin(), files_copy.end(), path);
      if (found != files_copy.end()) {
        std::remove(path.c_str());
        files_copy.erase(found);
        if (files_copy.size() == 1) {
          return false;  // Interrupt when only 1 file left
        }
      }
      return true;
    });
    TEST_EQUAL(size_t(1), files_copy.size());
    // At this point, only 1 file should left in the folder.
    for (const auto & file : files) {
      if (file == files_copy.front()) {
        TEST_EQUAL(file, FileManager::ReadFileAsString(file));
      } else {
        TEST_EXCEPTION(std::ios_base::failure, FileManager::ReadFileAsString(file))
      }
    }
  }
}

void Test_GetFileSize() {
  const std::string file = GenerateTemporaryFileName();
  ScopedRemoveFile remover(file);
  // File does not exist yet.
  TEST_EXCEPTION(std::ios_base::failure, FileManager::GetFileSize(file));
  // Use file name itself as a file contents.
  TEST_EQUAL(true, FileManager::AppendStringToFile(file, file));
  TEST_EQUAL(file.size(), FileManager::GetFileSize(file));
  // It should also fail for directories.
  TEST_EXCEPTION(std::ios_base::failure, FileManager::GetFileSize(FileManager::GetDirectoryFromFilePath(file)));
}
}

// ******************* Message Queue tests ******************

using alohalytics::HundredKilobytesFileQueue;
using alohalytics::ProcessingResult;

bool EndsWith(const std::string & str, const std::string & suffix) {
  const std::string::size_type str_size = str.size(), suffix_size = suffix.size();
  return str_size >= suffix_size && std::equal(suffix.begin(), suffix.end(), str.end() - suffix_size);
}

void Test_EndsWith() {
  TEST_EQUAL(true, EndsWith("", ""));
  TEST_EQUAL(true, EndsWith("Hello, World!", " World!"));
  TEST_EQUAL(true, EndsWith("Hello", "Hello"));
  TEST_EQUAL(false, EndsWith("Hello, World!", " World! "));
  TEST_EQUAL(false, EndsWith("Hell", "Hello"));
  TEST_EQUAL(false, EndsWith("ello", "Hello"));
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

void Test_MessagesQueue_InMemory_Empty() {
  bool processor_was_called = false;
  HundredKilobytesFileQueue q;
  FinishTask finish_task;
  q.ProcessArchivedFiles([&processor_was_called](bool, const std::string &) {
    processor_was_called = true;  // This code should not be executed.
    return false;
  }, std::bind(&FinishedCallback, std::placeholders::_1, std::ref(finish_task)));
  TEST_EQUAL(ProcessingResult::ENothingToProcess, finish_task.get());
  TEST_EQUAL(false, processor_was_called);
}

void Test_MessagesQueue_InMemory_SuccessfulProcessing() {
  HundredKilobytesFileQueue q;
  q.PushMessage(kTestMessage);
  std::thread worker([&q]() { q.PushMessage(kTestWorkerMessage); });
  worker.join();
  bool processor_was_called = false;
  FinishTask finish_task;
  q.ProcessArchivedFiles([&processor_was_called](bool is_file, const std::string & messages) {
    TEST_EQUAL(false, is_file);
    TEST_EQUAL(messages, kTestMessage + kTestWorkerMessage);
    processor_was_called = true;
    return true;
  }, std::bind(&FinishedCallback, std::placeholders::_1, std::ref(finish_task)));
  TEST_EQUAL(ProcessingResult::EProcessedSuccessfully, finish_task.get());
  TEST_EQUAL(true, processor_was_called);
}

void Test_MessagesQueue_InMemory_FailedProcessing() {
  HundredKilobytesFileQueue q;
  q.PushMessage(kTestMessage);
  bool processor_was_called = false;
  FinishTask finish_task;
  q.ProcessArchivedFiles([&processor_was_called](bool is_file, const std::string & messages) {
    TEST_EQUAL(false, is_file);
    TEST_EQUAL(messages, kTestMessage);
    processor_was_called = true;
    return false;
  }, std::bind(&FinishedCallback, std::placeholders::_1, std::ref(finish_task)));
  TEST_EQUAL(ProcessingResult::EProcessingError, finish_task.get());
  TEST_EQUAL(true, processor_was_called);
}

void Test_MessagesQueue_SwitchFromInMemoryToFile_and_OfflineEmulation() {
  const std::string tmpdir = FileManager::GetDirectoryFromFilePath(GenerateTemporaryFileName());
  CleanUpQueueFiles(tmpdir);
  ScopedRemoveFile remover(tmpdir + alohalytics::kCurrentFileName);
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
    TEST_EQUAL(kTestMessage + kTestWorkerMessage,
               FileManager::ReadFileAsString(tmpdir + alohalytics::kCurrentFileName));

    bool processor_was_called = false;
    FinishTask finish_task;
    HundredKilobytesFileQueue q;
    q.SetStorageDirectory(tmpdir);
    q.ProcessArchivedFiles([&processor_was_called, &archived_file](bool is_file, const std::string & full_file_path) {
      TEST_EQUAL(true, is_file);
      TEST_EQUAL(kTestMessage + kTestWorkerMessage, FileManager::ReadFileAsString(full_file_path));
      processor_was_called = true;
      archived_file = full_file_path;
      return false;  // Emulate network error.
    }, std::bind(&FinishedCallback, std::placeholders::_1, std::ref(finish_task)));
    TEST_EQUAL(ProcessingResult::EProcessingError, finish_task.get());
    TEST_EQUAL(true, processor_was_called);
    // Current file should be empty as it was archived for processing.
    TEST_EQUAL("", FileManager::ReadFileAsString(tmpdir + alohalytics::kCurrentFileName));
    TEST_EQUAL(kTestMessage + kTestWorkerMessage, FileManager::ReadFileAsString(archived_file));
  }

  // Create second archive in the queue after ProcessArchivedFiles() call.
  HundredKilobytesFileQueue q;
  q.SetStorageDirectory(tmpdir);
  q.PushMessage(kTestMessage);
  {
    bool archive1_processed = false, archive2_processed = false;
    FinishTask finish_task;
    q.ProcessArchivedFiles([&](bool is_file, const std::string & full_file_path) {
      TEST_EQUAL(true, is_file);
      if (full_file_path == archived_file) {
        TEST_EQUAL(kTestMessage + kTestWorkerMessage, FileManager::ReadFileAsString(full_file_path));
        archive1_processed = true;
      } else {
        TEST_EQUAL(kTestMessage, FileManager::ReadFileAsString(full_file_path));
        second_archived_file = full_file_path;
        archive2_processed = true;
      }
      return true;  // Archives should be deleted by queue after successful processing.
    }, std::bind(&FinishedCallback, std::placeholders::_1, std::ref(finish_task)));
    TEST_EQUAL(ProcessingResult::EProcessedSuccessfully, finish_task.get());
    TEST_EQUAL(true, archive1_processed);
    TEST_EQUAL(true, archive2_processed);
    TEST_EXCEPTION(std::ios_base::failure, FileManager::ReadFileAsString(archived_file));
    TEST_EXCEPTION(std::ios_base::failure, FileManager::ReadFileAsString(second_archived_file));
  }
}

void Test_MessagesQueue_CreateArchiveOnSizeLimitHit() {
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
    TEST_EQUAL(true, is_file);
    file_sizes.push_back(FileManager::ReadFileAsString(full_file_path).size());
    return true;
  }, std::bind(&FinishedCallback, std::placeholders::_1, std::ref(finish_task)));
  TEST_EQUAL(ProcessingResult::EProcessedSuccessfully, finish_task.get());
  TEST_EQUAL(size_t(2), file_sizes.size());
  TEST_EQUAL(size, file_sizes[0] + file_sizes[1]);
  TEST_EQUAL(true, (file_sizes[0] > q.kMaxFileSizeInBytes) != (file_sizes[1] > q.kMaxFileSizeInBytes));
}

void Test_MessagesQueue_HighLoadAndIntegrity() {
  // TODO(AlexZ): This test can be improved by generating really a lot of data
  // so many archives will be created. But it will make everything much more complex now.
  const std::string tmpdir = FileManager::GetDirectoryFromFilePath(GenerateTemporaryFileName());
  CleanUpQueueFiles(tmpdir);
  ScopedRemoveFile remover(tmpdir + alohalytics::kCurrentFileName);
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
  TEST_EQUAL(true, total_size > 0);
  for (auto & thread : threads) {
    thread.join();
  }
  FinishTask finish_task;
  q.ProcessArchivedFiles([&total_size](bool is_file, const std::string & full_file_path) {
    TEST_EQUAL(true, is_file);
    const std::string data = FileManager::ReadFileAsString(full_file_path);
    TEST_EQUAL(total_size, data.size());
    // Integrity check.
    size_t beg = 0, end = 0;
    while (beg < data.size()) {
      end += data[beg];
      const size_t count = end - beg;
      TEST_EQUAL(std::string(static_cast<size_t>(data[beg]), count), data.substr(beg, count));
      beg += count;
    }
    total_size = 0;
    return true;
  }, std::bind(&FinishedCallback, std::placeholders::_1, std::ref(finish_task)));
  TEST_EQUAL(ProcessingResult::EProcessedSuccessfully, finish_task.get());
  TEST_EQUAL(size_t(0), total_size);  // Zero means that processor was called.
}

int main(int, char * []) {
  // TODO(AlexZ): Split unit tests into two separate files.
  Test_ScopedRemoveFile();
  Test_GetDirectoryFromFilePath();
  Test_CreateTemporaryFile();
  Test_ReadFileAsString();
  Test_AppendStringToFile();
  Test_ForEachFileInDir();
  Test_GetFileSize();

  Test_EndsWith();
  Test_MessagesQueue_InMemory_Empty();
  Test_MessagesQueue_InMemory_SuccessfulProcessing();
  Test_MessagesQueue_InMemory_FailedProcessing();
  Test_MessagesQueue_SwitchFromInMemoryToFile_and_OfflineEmulation();
  Test_MessagesQueue_CreateArchiveOnSizeLimitHit();
  Test_MessagesQueue_HighLoadAndIntegrity();

  std::cout << "All tests have passed." << std::endl;
  return 0;
}

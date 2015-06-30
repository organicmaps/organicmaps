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

#include "test_defines.h"

#include <iostream>
#include <vector>

using alohalytics::FileManager;
using alohalytics::ScopedRemoveFile;

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
    const ScopedRemoveFile remover(file);
    TEST_EQUAL(true, FileManager::AppendStringToFile(file, file));
    TEST_EQUAL(file, FileManager::ReadFileAsString(file));
  }
  TEST_EXCEPTION(std::ios_base::failure, FileManager::ReadFileAsString(file));
}

void Test_CreateTemporaryFile() {
  const std::string file1 = GenerateTemporaryFileName();
  const ScopedRemoveFile remover1(file1);
  TEST_EQUAL(true, FileManager::AppendStringToFile(file1, file1));
  TEST_EQUAL(file1, FileManager::ReadFileAsString(file1));
  const std::string file2 = GenerateTemporaryFileName();
  TEST_EQUAL(false, file1 == file2);
  const ScopedRemoveFile remover2(file2);
  TEST_EQUAL(true, FileManager::AppendStringToFile(file2, file2));
  TEST_EQUAL(file2, FileManager::ReadFileAsString(file2));
  TEST_EQUAL(true, file1 != file2);
}

void Test_AppendStringToFile() {
  const std::string file = GenerateTemporaryFileName();
  const ScopedRemoveFile remover(file);
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
  const ScopedRemoveFile remover(file);
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
  const ScopedRemoveFile remover(file);
  // File does not exist yet.
  TEST_EXCEPTION(std::ios_base::failure, FileManager::GetFileSize(file));
  // Use file name itself as a file contents.
  TEST_EQUAL(true, FileManager::AppendStringToFile(file, file));
  TEST_EQUAL(file.size(), FileManager::GetFileSize(file));
  // It should also fail for directories.
  TEST_EXCEPTION(std::ios_base::failure, FileManager::GetFileSize(FileManager::GetDirectoryFromFilePath(file)));
}

void Test_IsDirectoryWritable() {
  const std::string file = GenerateTemporaryFileName();
  const ScopedRemoveFile remover(file);
  TEST_EQUAL(true, FileManager::IsDirectoryWritable(FileManager::GetDirectoryFromFilePath(file)));
  TEST_EQUAL(false, FileManager::IsDirectoryWritable(file));

  const std::string not_writable_system_directory =
#ifdef _MSC_VER
      "C:\\";
#else
      "/Users";
#endif
      // Suppose you do not run tests as root/Administrator.
      TEST_EQUAL(false, FileManager::IsDirectoryWritable(not_writable_system_directory));
}

int main(int, char * []) {
  Test_ScopedRemoveFile();
  Test_GetDirectoryFromFilePath();
  Test_CreateTemporaryFile();
  Test_ReadFileAsString();
  Test_AppendStringToFile();
  Test_ForEachFileInDir();
  Test_GetFileSize();
  Test_IsDirectoryWritable();

  std::cout << "All tests have passed." << std::endl;
  return 0;
}

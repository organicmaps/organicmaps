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

#include <iostream>
#include <memory>
#include <vector>

using alohalytics::FileManager;
using alohalytics::ScopedRemoveFile;

TEST(FileManager, GetDirectoryFromFilePath) {
  const std::string s = std::string(1, FileManager::kDirectorySeparator);
  const std::string ns = (s == "/") ? "\\" : "/";
  EXPECT_EQ("", FileManager::GetDirectoryFromFilePath(""));
  EXPECT_EQ(".", FileManager::GetDirectoryFromFilePath("some_file_name.ext"));
  EXPECT_EQ(".", FileManager::GetDirectoryFromFilePath("evil" + ns + "file"));
  EXPECT_EQ("dir" + s, FileManager::GetDirectoryFromFilePath("dir" + s + "file"));
  EXPECT_EQ(s + "root" + s + "dir" + s, FileManager::GetDirectoryFromFilePath(s + "root" + s + "dir" + s + "file"));
  EXPECT_EQ(".", FileManager::GetDirectoryFromFilePath("dir" + ns + "file"));
  EXPECT_EQ("C:" + s + "root" + s + "dir" + s,
            FileManager::GetDirectoryFromFilePath("C:" + s + "root" + s + "dir" + s + "file.ext"));
  EXPECT_EQ(s + "tmp" + s, FileManager::GetDirectoryFromFilePath(s + "tmp" + s + "evil" + ns + "file"));
}

TEST(FileManager, ScopedRemoveFile) {
  const std::string file = GenerateTemporaryFileName();
  {
    const ScopedRemoveFile remover(file);
    EXPECT_TRUE(FileManager::AppendStringToFile(file, file));
    EXPECT_EQ(file, FileManager::ReadFileAsString(file));
  }
  EXPECT_THROW(FileManager::ReadFileAsString(file), std::ios_base::failure);
}

TEST(FileManager, CreateTemporaryFile) {
  const std::string file1 = GenerateTemporaryFileName();
  const ScopedRemoveFile remover1(file1);
  EXPECT_TRUE(FileManager::AppendStringToFile(file1, file1));
  EXPECT_EQ(file1, FileManager::ReadFileAsString(file1));
  const std::string file2 = GenerateTemporaryFileName();
  EXPECT_NE(file1, file2);
  const ScopedRemoveFile remover2(file2);
  EXPECT_TRUE(FileManager::AppendStringToFile(file2, file2));
  EXPECT_EQ(file2, FileManager::ReadFileAsString(file2));
  EXPECT_NE(file1, file2);
}

TEST(FileManager, AppendStringToFile) {
  const std::string file = GenerateTemporaryFileName();
  const ScopedRemoveFile remover(file);
  const std::string s1("First\0 String");
  EXPECT_TRUE(FileManager::AppendStringToFile(s1, file));
  EXPECT_EQ(s1, FileManager::ReadFileAsString(file));
  const std::string s2("Second one.");
  EXPECT_TRUE(FileManager::AppendStringToFile(s2, file));
  EXPECT_EQ(s1 + s2, FileManager::ReadFileAsString(file));

  EXPECT_FALSE(FileManager::AppendStringToFile(file, ""));
}

TEST(FileManager, ReadFileAsString) {
  const std::string file = GenerateTemporaryFileName();
  const ScopedRemoveFile remover(file);
  EXPECT_TRUE(FileManager::AppendStringToFile(file, file));
  EXPECT_EQ(file, FileManager::ReadFileAsString(file));
}

TEST(FileManager, ForEachFileInDir) {
  {
    bool was_called_at_least_once = false;
    FileManager::ForEachFileInDir("", [&was_called_at_least_once](const std::string &) -> bool {
      was_called_at_least_once = true;
      return true;
    });
    EXPECT_FALSE(was_called_at_least_once);
  }

  {
    std::vector<std::string> files, files_copy;
    std::vector<std::unique_ptr<ScopedRemoveFile>> removers;
    for (size_t i = 0; i < 5; ++i) {
      const std::string file = GenerateTemporaryFileName();
      files.push_back(file);
      removers.emplace_back(new ScopedRemoveFile(file));
      EXPECT_TRUE(FileManager::AppendStringToFile(file, file));
    }
    files_copy = files;
    const std::string directory = FileManager::GetDirectoryFromFilePath(files[0]);
    EXPECT_FALSE(directory.empty());
    FileManager::ForEachFileInDir(directory, [&files_copy](const std::string & path) -> bool {
      // Some random files can remain in the temporary directory.
      const auto found = std::find(files_copy.begin(), files_copy.end(), path);
      if (found != files_copy.end()) {
        EXPECT_EQ(path, FileManager::ReadFileAsString(path));
        files_copy.erase(found);
      }
      return true;
    });
    EXPECT_EQ(size_t(0), files_copy.size());

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
    EXPECT_EQ(size_t(1), files_copy.size());
    // At this point, only 1 file should left in the folder.
    for (const auto & file : files) {
      if (file == files_copy.front()) {
        EXPECT_EQ(file, FileManager::ReadFileAsString(file));
      } else {
        EXPECT_THROW(FileManager::ReadFileAsString(file), std::ios_base::failure);
      }
    }
  }
}

TEST(FileManager, GetFileSize) {
  const std::string file = GenerateTemporaryFileName();
  const ScopedRemoveFile remover(file);
  // File does not exist yet.
  EXPECT_THROW(FileManager::GetFileSize(file), std::ios_base::failure);
  // Use file name itself as a file contents.
  EXPECT_TRUE(FileManager::AppendStringToFile(file, file));
  EXPECT_EQ(file.size(), FileManager::GetFileSize(file));
  // It should also fail for directories.
  EXPECT_THROW(FileManager::GetFileSize(FileManager::GetDirectoryFromFilePath(file)), std::ios_base::failure);
}

TEST(FileManager, IsDirectoryWritable) {
  const std::string file = GenerateTemporaryFileName();
  const ScopedRemoveFile remover(file);
  EXPECT_TRUE(FileManager::IsDirectoryWritable(FileManager::GetDirectoryFromFilePath(file)));
  EXPECT_FALSE(FileManager::IsDirectoryWritable(file));

  const std::string not_writable_system_directory =
#ifdef _MSC_VER
      "C:\\";
#else
      "/Users";
#endif
  // Suppose you do not run tests as root/Administrator.
  EXPECT_FALSE(FileManager::IsDirectoryWritable(not_writable_system_directory));
}

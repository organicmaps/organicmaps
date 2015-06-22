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

#ifndef FILE_MANAGER_H
#define FILE_MANAGER_H

#include <algorithm>
#include <functional>
#include <fstream>
#include <string>

namespace alohalytics {

// Useful helper.
struct ScopedRemoveFile {
  std::string file;
  ScopedRemoveFile(const std::string & file_to_delete) : file(file_to_delete) {}
  ~ScopedRemoveFile() { std::remove(file.c_str()); }
};

// Functions are wrapped into the class for convenience.
struct FileManager {
  // Initialized separately for each platform.
  static const char kDirectorySeparator;

  // Checks and appends if necessary platform-dependent slash at the end of the path.
  static void AppendDirectorySlash(std::string & directory) {
    // Fix missing slash if necessary.
    if (!directory.empty() && directory.back() != kDirectorySeparator) {
      directory.push_back(kDirectorySeparator);
    }
  }

  // Returns empty string for empty file_path.
  // Returns "." if file_path contains file name only.
  // Otherwise returns path with a slash at the end and without file name in it.
  static std::string GetDirectoryFromFilePath(std::string file_path) {
    if (file_path.empty()) {
      return std::string();
    }
    const std::string::size_type slash = file_path.find_last_of(kDirectorySeparator);
    if (slash == std::string::npos) {
      return std::string(".");
    }
    return file_path.erase(slash + 1);
  }

  // Creates a new file or appends string to an existing one.
  // Returns false on error.
  // TODO(AlexZ): Should consider exceptions?
  static bool AppendStringToFile(const std::string & str, const std::string & file_path) {
    return std::ofstream(file_path, std::ios_base::app | std::ios_base::binary | std::ios_base::out)
        .write(str.data(), str.size())
        .flush()
        .good();
  }

  // Returns file content as a string.
  // Throws ios_base::failure exception if file is absent or is a directory.
  static std::string ReadFileAsString(const std::string & file_path) {
    const uint64_t size = GetFileSize(file_path);
    std::ifstream fi(file_path, std::ios_base::binary | std::ios_base::in);
    fi.exceptions(std::ios_base::badbit | std::ios_base::failbit);
    std::string buffer(static_cast<const size_t>(size), '\0');
    fi.read(&buffer[0], static_cast<std::streamsize>(size));
    return buffer;
  }

  // Returns true if we can write to the specified directory.
  static bool IsDirectoryWritable(std::string directory) {
    AppendDirectorySlash(directory);
    std::string temporary_file = directory;
    // For a pseudo-random file we turn directory path into a file name.
    std::replace(temporary_file.begin(), temporary_file.end(), kDirectorySeparator, 'Z');
    // Make sure it does not exist first.
    while (true) {
      try {
        (void)GetFileSize(directory + temporary_file);
        temporary_file += 'Z';
      } catch (const std::exception &) {
        break;
      }
    }
    const ScopedRemoveFile remover(directory + temporary_file);
    bool is_writable = false;
    try {
      (void)FileManager::AppendStringToFile(temporary_file, remover.file);
      if (FileManager::ReadFileAsString(remover.file) == temporary_file) {
        is_writable = true;
      }
    } catch (const std::exception &) {
    }
    return is_writable;
  }

  // Executes lambda for each regular file in the directory and stops immediately if lambda returns false.
  static void ForEachFileInDir(std::string directory, std::function<bool(const std::string & full_path)> lambda);

  // Throws std::ios_base::failure exception if file is absent or is a directory.
  static uint64_t GetFileSize(const std::string & full_path_to_file);
};

}  // namespace alohalytics

#endif  // FILE_MANAGER_H

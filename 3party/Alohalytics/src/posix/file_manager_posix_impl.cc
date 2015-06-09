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

// POSIX implementations for FileManager
#include "../file_manager.h"

#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>

namespace alohalytics {

const char FileManager::kDirectorySeparator = '/';

namespace {
struct ScopedCloseDir {
  DIR * dir_;
  ScopedCloseDir(DIR * dir) : dir_(dir) {}
  ~ScopedCloseDir() { ::closedir(dir_); }
};
}  // namespace

void FileManager::ForEachFileInDir(std::string directory, std::function<bool(const std::string & full_path)> lambda) {
  // Silently ignore invalid directories.
  if (directory.empty()) {
    return;
  }
  DIR * dir = ::opendir(directory.c_str());
  if (!dir) {
    return;
  }
  AppendDirectorySlash(directory);
  const ScopedCloseDir closer(dir);
  while (struct dirent * entry = ::readdir(dir)) {
    if (!(entry->d_type & DT_DIR)) {
      if (!lambda(directory + entry->d_name)) {
        return;
      }
    }
  }
}

int64_t FileManager::GetFileSize(const std::string & full_path_to_file) {
  struct stat st;
  if (::stat(full_path_to_file.c_str(), &st) || S_ISDIR(st.st_mode)) {
    return -1;
  }
  return static_cast<int64_t>(st.st_size);
}
}  // namespace alohalytics

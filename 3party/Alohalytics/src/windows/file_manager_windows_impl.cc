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

// Windows implementations for FileManager.
#include <windows.h>
#include <direct.h>
#include <stdio.h>

namespace alohalytics {

const char FileManager::kDirectorySeparator = '\\';

namespace {
template <class TCloseFunc>
struct ScopedCloseFindFileHandle {
  HANDLE handle_;
  TCloseFunc closer_;
  ScopedCloseFindFileHandle(HANDLE handle, TCloseFunc closer) : handle_(handle), closer_(closer) {}
  ~ScopedCloseFindFileHandle() { closer_(handle_); }
};
}  // namespace

void FileManager::ForEachFileInDir(std::string directory, std::function<bool(const std::string & full_path)> lambda) {
  // Silently ignore invalid (empty) directories.
  if (directory.empty()) {
    return;
  }
  AppendDirectorySlash(directory);
  // TODO(AlexZ): Do we need to use *W functions (and wstrings) for files processing?
  WIN32_FIND_DATAA find_data;
  HANDLE handle = ::FindFirstFileA((directory + "*.*").c_str(), &find_data);
  if (handle == INVALID_HANDLE_VALUE) {
    return;
  }
  const ScopedCloseFindFileHandle closer(handle, &::FindClose);
  do {
    if (!(find_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
      if (!lambda(directory + find_data.cFileName)) {
        // Stop processing if lambda has returned false.
        return;
      }
    }
  } while (::FindNextFileA(handle, &find_data) != 0);
}

int64_t FileManager::GetFileSize(const std::string & full_path_to_file) {
  HANDLE handle = ::CreateFileA(full_path_to_file.c_str(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING,
                                FILE_ATTRIBUTE_NORMAL, NULL);
  if (handle != INVALID_HANDLE_VALUE) {
    const ScopedCloseFindFileHandle closer(handle, &::CloseHandle);
    LARGE_INTEGER size;
    if (0 != GetFileSizeEx(hFile, &size)) {
      return static_cast<int64_t>(size.QuadPart);
    }
  }
  return -1;
}

}  // namespace alohalytics

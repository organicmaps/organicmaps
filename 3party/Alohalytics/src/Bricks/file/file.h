// TODO(dkorolev): Add unit tests.
// TODO(dkorolev): Move everything under bricks::file::FileSystem and have all the tests pass.

#ifndef BRICKS_FILE_FILE_H
#define BRICKS_FILE_FILE_H

/*******************************************************************************
The MIT License (MIT)

Copyright (c) 2014 Alexander Zolotarev <me@alex.bio> from Minsk, Belarus
          (c) 2014 Dmitry "Dima" Korolev <dmitry.korolev@gmail.com>

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

#include <fstream>
#include <string>
#include <cstring>

#ifdef _MSC_VER
#include <windows.h>
#include <direct.h>
struct ScopedCloseFindFileHandle {
  HANDLE handle_;
  ScopedCloseFindFileHandle(HANDLE handle) : handle_(handle) {}
  ~ScopedCloseFindFileHandle() {
    ::FindClose(handle_);
  }
};
#else
#include <dirent.h>
#include <sys/stat.h>
struct ScopedCloseDir {
  DIR * dir_;
  ScopedCloseDir(DIR * dir) : dir_(dir) {}
  ~ScopedCloseDir() {
    ::closedir(dir_);
  }
};
#endif

#include "exceptions.h"
#include "../../logger.h"

namespace bricks {

inline std::string ReadFileAsString(std::string const& file_name) {
  try {
    std::ifstream fi;
    fi.exceptions(std::ifstream::failbit | std::ifstream::badbit);
    fi.open(file_name, std::ifstream::binary);
    fi.seekg(0, std::ios::end);
    const std::streampos size = fi.tellg();
    std::string buffer(static_cast<const size_t>(size), '\0');
    fi.seekg(0);
    if (fi.read(&buffer[0], size).good()) {
      return buffer;
    } else {
      // TODO(dkorolev): Ask Alex whether there's a better way than what I have here with two exceptions.
      throw FileException();
    }
  } catch (const std::ifstream::failure&) {
    throw FileException();
  } catch (FileException()) {
    throw FileException();
  }
}

inline void WriteStringToFile(const std::string& file_name, const std::string& contents, bool append = false) {
  try {
    std::ofstream fo;
    fo.exceptions(std::ofstream::failbit | std::ofstream::badbit);
    fo.open(file_name, (append ? std::ofstream::app : std::ofstream::trunc) | std::ofstream::binary);
    fo << contents;
  } catch (const std::ofstream::failure&) {
    throw FileException();
  }
}

enum class RemoveFileParameters { ThrowExceptionOnError, Silent };
inline void RemoveFile(const std::string& file_name,
                       RemoveFileParameters parameters = RemoveFileParameters::ThrowExceptionOnError) {
  if (::remove(file_name.c_str())) {
    if (parameters == RemoveFileParameters::ThrowExceptionOnError) {
      throw FileException();
    }
  }
}

class ScopedRemoveFile final {
 public:
  explicit ScopedRemoveFile(const std::string& file_name, bool remove_now_as_well = true)
      : file_name_(file_name) {
    if (remove_now_as_well) {
      RemoveFile(file_name_, RemoveFileParameters::Silent);
    }
  }
  ~ScopedRemoveFile() {
    RemoveFile(file_name_, RemoveFileParameters::Silent);
  }

 private:
  std::string file_name_;
};

// Platform-indepenent, injection-friendly filesystem wrapper.
struct FileSystem {
  typedef std::ofstream OutputFile;

  static inline std::string ReadFileAsString(std::string const& file_name) {
    return bricks::ReadFileAsString(file_name);
  }

  static inline void WriteStringToFile(const std::string& file_name,
                                       const std::string& contents,
                                       bool append = false) {
    bricks::WriteStringToFile(file_name, contents, append);
  }

  static inline std::string JoinPath(const std::string& path_name, const std::string& base_name) {
    if (path_name.empty()) {
      return base_name;
    } else if (path_name.back() == '/') {
      return path_name + base_name;
    } else {
      return path_name + '/' + base_name;
    }
  }

  static inline void RenameFile(const std::string& old_name, const std::string& new_name) {
    if (0 != ::rename(old_name.c_str(), new_name.c_str())) {
      ALOG("Error", errno, "renaming file", old_name, "to", new_name);
    }
  }

  static inline void RemoveFile(const std::string& file_name,
                                RemoveFileParameters parameters = RemoveFileParameters::ThrowExceptionOnError) {
    bricks::RemoveFile(file_name, parameters);
  }

  static inline void ScanDirUntil(const std::string& directory,
                                  std::function<bool(const std::string&)> lambda) {
#ifdef _MSC_VER
    WIN32_FIND_DATAA find_data;
    HANDLE handle = ::FindFirstFileA(directory.c_str(), &find_data);
    if (handle == INVALID_HANDLE_VALUE) {
      return;
    }
    const ScopedCloseFindFileHandle closer(handle);
    do {
      if (!(find_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
        if (!lambda(find_data.cFileName)) {
          return;
        }
      }
    } while (::FindNextFileA(handle, &find_data) != 0);
#else
    DIR * dir = ::opendir(directory.c_str());
    if (dir) {
      const ScopedCloseDir closer(dir);
      while (struct dirent* entry = ::readdir(dir)) {
        if (*entry->d_name && ::strcmp(entry->d_name, ".") && ::strcmp(entry->d_name, "..")) {
          if (!lambda(entry->d_name)) {
            return;
          }
        }
      }
    }
#endif
  }

  static inline void ScanDir(const std::string& directory, std::function<void(const std::string&)> lambda) {
    ScanDirUntil(directory, [lambda](const std::string& filename) {
      lambda(filename);
      return true;
    });
  }

  static inline uint64_t GetFileSize(const std::string& file_name) {
    struct stat info;
    if (stat(file_name.c_str(), &info)) {
      // TODO(dkorolev): Throw an exception and analyze errno.
      return 0;
    } else {
      return static_cast<uint64_t>(info.st_size);
    }
  }

  static inline void CreateDirectory(const std::string& directory) {
#ifdef _MSC_VER
    ::_mkdir(directory.c_str());
#else
    // Hard-code default permissions to avoid cross-platform compatibility issues.
    ::mkdir(directory.c_str(), 0755);
#endif
  }
};

}  // namespace bricks

#endif  // BRICKS_FILE_FILE_H

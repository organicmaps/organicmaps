#pragma once

#include "base/assert.hpp"

#include <cstdint>
#include <string>
#include <utility>

namespace downloader
{
enum class DownloadStatus
{
  InProgress,
  Completed,
  Failed,
  FileNotFound
};

inline std::string DebugPrint(DownloadStatus status)
{
  switch (status)
  {
  case DownloadStatus::InProgress: return "In progress";
  case DownloadStatus::Completed: return "Completed";
  case DownloadStatus::Failed: return "Failed";
  case DownloadStatus::FileNotFound: return "File not found";
  }
  UNREACHABLE();
}

/// <bytes downloaded, total number of bytes>, total can be -1 if size is unknown
using Progress = std::pair<int64_t, int64_t>;
}  // namespace downloader

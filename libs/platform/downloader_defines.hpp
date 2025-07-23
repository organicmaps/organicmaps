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
  FileNotFound,
  FailedSHA,
};

inline std::string DebugPrint(DownloadStatus status)
{
  switch (status)
  {
  case DownloadStatus::InProgress: return "In progress";
  case DownloadStatus::Completed: return "Completed";
  case DownloadStatus::Failed: return "Failed";
  case DownloadStatus::FileNotFound: return "File not found";
  case DownloadStatus::FailedSHA: return "Failed SHA check";
  }
  UNREACHABLE();
}

struct Progress
{
  static int64_t constexpr kUnknownTotalSize = -1;

  static Progress constexpr Unknown() { return {0, kUnknownTotalSize}; }

  bool IsUnknown() const { return m_bytesTotal == kUnknownTotalSize; }

  int64_t m_bytesDownloaded = 0;
  /// Total can be kUnknownTotalSize if size is unknown.
  int64_t m_bytesTotal = 0;
};

inline std::string DebugPrint(Progress const & progress)
{
  std::ostringstream out;
  out << "(downloaded " << progress.m_bytesDownloaded << " bytes out of " << progress.m_bytesTotal << " bytes)";
  return out.str();
}
}  // namespace downloader

#pragma once

#include "base/macros.hpp"

#include <string>

namespace om::network
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
}  // namespace om::network

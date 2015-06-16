#pragma once

#include "std/cstdint.hpp"
#include "std/string.hpp"
#include "std/utility.hpp"

namespace storage
{
  /// Used in GUI
  enum class TStatus : uint8_t
  {
    EOnDisk = 0,
    ENotDownloaded,
    EDownloadFailed,
    EDownloading,
    EInQueue,
    EUnknown,
    EOnDiskOutOfDate,
    EOutOfMemFailed  // EDownloadFailed because not enough memory
  };

  string DebugPrint(TStatus status);

  typedef pair<uint64_t, uint64_t> LocalAndRemoteSizeT;
  }  // namespace storage

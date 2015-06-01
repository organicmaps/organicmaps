#pragma once

#include "3party/enum_flags.hpp"

#include "std/stdint.hpp"
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

  ENUM_FLAGS(TMapOptions)
  enum class TMapOptions
  {
    EMap = 0x1,
    ECarRouting = 0x2,
    EMapWithCarRouting = 0x3
  };

  typedef pair<uint64_t, uint64_t> LocalAndRemoteSizeT;
}

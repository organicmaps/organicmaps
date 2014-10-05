#pragma once

#include "../std/stdint.hpp"
#include "../std/utility.hpp"

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

  enum class TMapOptions : uint8_t
  {
    EMapOnly = 0x1,
    ECarRouting = 0x2,
    EMapWithCarRouting = 0x3
  };

  inline TMapOptions operator | (TMapOptions const & lhs, TMapOptions const & rhs)
  {
    return static_cast<TMapOptions>(static_cast<int>(lhs) | static_cast<int>(rhs));
  }

  inline TMapOptions & operator |= (TMapOptions & lhs, TMapOptions rhs)
  {
    return lhs = static_cast<TMapOptions>(static_cast<int>(lhs) | static_cast<int>(rhs));
  }

  inline bool operator & (TMapOptions const & testedFlags, TMapOptions const & match)
  {
    return (static_cast<int>(testedFlags) & static_cast<int>(match)) != 0;
  }

  typedef pair<uint64_t, uint64_t> LocalAndRemoteSizeT;
}

#include "storage/storage_defines.hpp"

namespace storage
{
string DebugPrint(TStatus status)
{
  switch (status)
  {
  case TStatus::EUndefined:
    return string("EUndefined");
  case TStatus::EOnDisk:
    return string("OnDisk");
  case TStatus::ENotDownloaded:
    return string("NotDownloaded");
  case TStatus::EDownloadFailed:
    return string("DownloadFailed");
  case TStatus::EDownloading:
    return string("Downloading");
  case TStatus::EInQueue:
    return string("InQueue");
  case TStatus::EUnknown:
    return string("Unknown");
  case TStatus::EOnDiskOutOfDate:
    return string("OnDiskOutOfDate");
  case TStatus::EOutOfMemFailed:
    return string("OutOfMemFailed");
  case TStatus::EMixed:
    return string("EMixed");
  }
}
}  // namespace storage

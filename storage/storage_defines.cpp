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

string DebugPrint(TNodeStatus status)
{
  switch (status)
  {
  case TNodeStatus::Undefined:
    return string("Undefined");
  case TNodeStatus::Error:
    return string("Error");
  case TNodeStatus::OnDisk:
    return string("OnDisk");
  case TNodeStatus::NotDownloaded:
    return string("NotDownloaded");
  case TNodeStatus::Downloading:
    return string("Downloading");
  case TNodeStatus::InQueue:
    return string("InQueue");
  case TNodeStatus::OnDiskOutOfDate:
    return string("OnDiskOutOfDate");
  case TNodeStatus::Mixed:
    return string("Mixed");
  }
}

string DebugPrint(TErrNodeStatus status)
{
  switch (status)
  {
  case TErrNodeStatus::NoError:
    return string("NoError");
  case TErrNodeStatus::UnknownError:
    return string("UnknownError");
  case TErrNodeStatus::OutOfMemFailed:
    return string("OutOfMemFailed");
  case TErrNodeStatus::NoInetConnection:
    return string("NoInetConnection");
  }
}

TStatusAndError ParseStatus(TStatus innerStatus)
{
  switch (innerStatus)
  {
  case TStatus::EUndefined:
    return TStatusAndError(TNodeStatus::Undefined, TErrNodeStatus::NoError);
  case TStatus::EOnDisk:
    return TStatusAndError(TNodeStatus::OnDisk, TErrNodeStatus::NoError);
  case TStatus::ENotDownloaded:
    return TStatusAndError(TNodeStatus::NotDownloaded, TErrNodeStatus::NoError);
  case TStatus::EDownloadFailed:
    return TStatusAndError(TNodeStatus::Error, TErrNodeStatus::NoInetConnection);
  case TStatus::EDownloading:
    return TStatusAndError(TNodeStatus::Downloading, TErrNodeStatus::NoError);
  case TStatus::EInQueue:
    return TStatusAndError(TNodeStatus::InQueue, TErrNodeStatus::NoError);
  case TStatus::EUnknown:
    return TStatusAndError(TNodeStatus::Error, TErrNodeStatus::UnknownError);
  case TStatus::EOnDiskOutOfDate:
    return TStatusAndError(TNodeStatus::OnDiskOutOfDate, TErrNodeStatus::NoError);
  case TStatus::EOutOfMemFailed:
    return TStatusAndError(TNodeStatus::Error, TErrNodeStatus::OutOfMemFailed);
  case TStatus::EMixed:
    return TStatusAndError(TNodeStatus::Mixed, TErrNodeStatus::NoError);
  }
}
}  // namespace storage

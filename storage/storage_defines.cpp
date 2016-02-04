#include "storage/storage_defines.hpp"

#include "std/sstream.hpp"

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

string DebugPrint(TNodeErrorCode status)
{
  switch (status)
  {
  case TNodeErrorCode::NoError:
    return string("NoError");
  case TNodeErrorCode::UnknownError:
    return string("UnknownError");
  case TNodeErrorCode::OutOfMemFailed:
    return string("OutOfMemFailed");
  case TNodeErrorCode::NoInetConnection:
    return string("NoInetConnection");
  }
}

StatusAndError ParseStatus(TStatus innerStatus)
{
  switch (innerStatus)
  {
  case TStatus::EUndefined:
    return StatusAndError(TNodeStatus::Undefined, TNodeErrorCode::NoError);
  case TStatus::EOnDisk:
    return StatusAndError(TNodeStatus::OnDisk, TNodeErrorCode::NoError);
  case TStatus::ENotDownloaded:
    return StatusAndError(TNodeStatus::NotDownloaded, TNodeErrorCode::NoError);
  case TStatus::EDownloadFailed:
    return StatusAndError(TNodeStatus::Error, TNodeErrorCode::NoInetConnection);
  case TStatus::EDownloading:
    return StatusAndError(TNodeStatus::Downloading, TNodeErrorCode::NoError);
  case TStatus::EInQueue:
    return StatusAndError(TNodeStatus::InQueue, TNodeErrorCode::NoError);
  case TStatus::EUnknown:
    return StatusAndError(TNodeStatus::Error, TNodeErrorCode::UnknownError);
  case TStatus::EOnDiskOutOfDate:
    return StatusAndError(TNodeStatus::OnDiskOutOfDate, TNodeErrorCode::NoError);
  case TStatus::EOutOfMemFailed:
    return StatusAndError(TNodeStatus::Error, TNodeErrorCode::OutOfMemFailed);
  case TStatus::EMixed:
    return StatusAndError(TNodeStatus::Mixed, TNodeErrorCode::NoError);
  }
}

string DebugPrint(StatusAndError statusAndError)
{
  ostringstream out;
  out << "StatusAndError[" << DebugPrint(statusAndError.status)
      << ", " << DebugPrint(statusAndError.error) << "]";
  return out.str();
}
}  // namespace storage

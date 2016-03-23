#include "storage/storage_defines.hpp"

#include "std/sstream.hpp"

namespace storage
{
string DebugPrint(Status status)
{
  switch (status)
  {
  case Status::EUndefined:
    return string("EUndefined");
  case Status::EOnDisk:
    return string("OnDisk");
  case Status::ENotDownloaded:
    return string("NotDownloaded");
  case Status::EDownloadFailed:
    return string("DownloadFailed");
  case Status::EDownloading:
    return string("Downloading");
  case Status::EInQueue:
    return string("InQueue");
  case Status::EUnknown:
    return string("Unknown");
  case Status::EOnDiskOutOfDate:
    return string("OnDiskOutOfDate");
  case Status::EOutOfMemFailed:
    return string("OutOfMemFailed");
  }
}

string DebugPrint(NodeStatus status)
{
  switch (status)
  {
  case NodeStatus::Undefined:
    return string("Undefined");
  case NodeStatus::Error:
    return string("Error");
  case NodeStatus::OnDisk:
    return string("OnDisk");
  case NodeStatus::NotDownloaded:
    return string("NotDownloaded");
  case NodeStatus::Downloading:
    return string("Downloading");
  case NodeStatus::InQueue:
    return string("InQueue");
  case NodeStatus::OnDiskOutOfDate:
    return string("OnDiskOutOfDate");
  case NodeStatus::Partly:
    return string("Partly");
  }
}

string DebugPrint(NodeErrorCode status)
{
  switch (status)
  {
  case NodeErrorCode::NoError:
    return string("NoError");
  case NodeErrorCode::UnknownError:
    return string("UnknownError");
  case NodeErrorCode::OutOfMemFailed:
    return string("OutOfMemFailed");
  case NodeErrorCode::NoInetConnection:
    return string("NoInetConnection");
  }
}

StatusAndError ParseStatus(Status innerStatus)
{
  switch (innerStatus)
  {
  case Status::EUndefined:
    return StatusAndError(NodeStatus::Undefined, NodeErrorCode::NoError);
  case Status::EOnDisk:
    return StatusAndError(NodeStatus::OnDisk, NodeErrorCode::NoError);
  case Status::ENotDownloaded:
    return StatusAndError(NodeStatus::NotDownloaded, NodeErrorCode::NoError);
  case Status::EDownloadFailed:
    return StatusAndError(NodeStatus::Error, NodeErrorCode::NoInetConnection);
  case Status::EDownloading:
    return StatusAndError(NodeStatus::Downloading, NodeErrorCode::NoError);
  case Status::EInQueue:
    return StatusAndError(NodeStatus::InQueue, NodeErrorCode::NoError);
  case Status::EUnknown:
    return StatusAndError(NodeStatus::Error, NodeErrorCode::UnknownError);
  case Status::EOnDiskOutOfDate:
    return StatusAndError(NodeStatus::OnDiskOutOfDate, NodeErrorCode::NoError);
  case Status::EOutOfMemFailed:
    return StatusAndError(NodeStatus::Error, NodeErrorCode::OutOfMemFailed);
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

#include "storage/storage_defines.hpp"

#include "base/assert.hpp"

#include <sstream>

using namespace std;
using namespace string_literals;

namespace storage
{
string DebugPrint(Status status)
{
  switch (status)
  {
  case Status::EUndefined:
    return "EUndefined"s;
  case Status::EOnDisk:
    return "OnDisk"s;
  case Status::ENotDownloaded:
    return "NotDownloaded"s;
  case Status::EDownloadFailed:
    return "DownloadFailed"s;
  case Status::EDownloading:
    return "Downloading"s;
  case Status::EApplying:
    return "Applying"s;
  case Status::EInQueue:
    return "InQueue"s;
  case Status::EUnknown:
    return "Unknown"s;
  case Status::EOnDiskOutOfDate:
    return "OnDiskOutOfDate"s;
  case Status::EOutOfMemFailed:
    return "OutOfMemFailed"s;
  }
  UNREACHABLE();
}

string DebugPrint(NodeStatus status)
{
  switch (status)
  {
  case NodeStatus::Undefined:
    return "Undefined"s;
  case NodeStatus::Error:
    return "Error"s;
  case NodeStatus::OnDisk:
    return "OnDisk"s;
  case NodeStatus::NotDownloaded:
    return "NotDownloaded"s;
  case NodeStatus::Downloading:
    return "Downloading"s;
  case NodeStatus::Applying:
    return "Applying"s;
  case NodeStatus::InQueue:
    return "InQueue"s;
  case NodeStatus::OnDiskOutOfDate:
    return "OnDiskOutOfDate"s;
  case NodeStatus::Partly:
    return "Partly"s;
  }
  UNREACHABLE();
}

string DebugPrint(NodeErrorCode status)
{
  switch (status)
  {
  case NodeErrorCode::NoError:
    return "NoError"s;
  case NodeErrorCode::UnknownError:
    return "UnknownError"s;
  case NodeErrorCode::OutOfMemFailed:
    return "OutOfMemFailed"s;
  case NodeErrorCode::NoInetConnection:
    return "NoInetConnection"s;
  }
  UNREACHABLE();
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
  case Status::EApplying:
    return StatusAndError(NodeStatus::Applying, NodeErrorCode::NoError);
  case Status::EInQueue:
    return StatusAndError(NodeStatus::InQueue, NodeErrorCode::NoError);
  case Status::EUnknown:
    return StatusAndError(NodeStatus::Error, NodeErrorCode::UnknownError);
  case Status::EOnDiskOutOfDate:
    return StatusAndError(NodeStatus::OnDiskOutOfDate, NodeErrorCode::NoError);
  case Status::EOutOfMemFailed:
    return StatusAndError(NodeStatus::Error, NodeErrorCode::OutOfMemFailed);
  }
  UNREACHABLE();
}

string DebugPrint(StatusAndError statusAndError)
{
  ostringstream out;
  out << "StatusAndError[" << DebugPrint(statusAndError.status)
      << ", " << DebugPrint(statusAndError.error) << "]";
  return out.str();
}
}  // namespace storage

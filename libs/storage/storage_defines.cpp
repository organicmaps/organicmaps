#include "storage/storage_defines.hpp"

#include "base/assert.hpp"

#include <sstream>
#include <string>

namespace storage
{
std::string_view DebugPrint(Status status)
{
  switch (status)
  {
    using enum Status;
  case Undefined: return "EUndefined";
  case OnDisk: return "OnDisk";
  case NotDownloaded: return "NotDownloaded";
  case DownloadFailed: return "DownloadFailed";
  case Downloading: return "Downloading";
  case Applying: return "Applying";
  case InQueue: return "InQueue";
  case UnknownError: return "Unknown";
  case OnDiskOutOfDate: return "OnDiskOutOfDate";
  case OutOfMemFailed: return "OutOfMemFailed";
  }
  UNREACHABLE();
}

std::string_view DebugPrint(NodeStatus status)
{
  switch (status)
  {
    using enum NodeStatus;
  case Undefined: return "Undefined";
  case Error: return "Error";
  case OnDisk: return "OnDisk";
  case NotDownloaded: return "NotDownloaded";
  case Downloading: return "Downloading";
  case Applying: return "Applying";
  case InQueue: return "InQueue";
  case OnDiskOutOfDate: return "OnDiskOutOfDate";
  case Partly: return "Partly";
  }
  UNREACHABLE();
}

std::string_view DebugPrint(NodeErrorCode status)
{
  switch (status)
  {
    using enum NodeErrorCode;
  case NoError: return "NoError";
  case UnknownError: return "UnknownError";
  case OutOfMemFailed: return "OutOfMemFailed";
  case NoInetConnection: return "NoInetConnection";
  }
  UNREACHABLE();
}

StatusAndError ParseStatus(Status innerStatus)
{
  switch (innerStatus)
  {
  case Status::Undefined: return StatusAndError(NodeStatus::Undefined, NodeErrorCode::NoError);
  case Status::OnDisk: return StatusAndError(NodeStatus::OnDisk, NodeErrorCode::NoError);
  case Status::NotDownloaded: return StatusAndError(NodeStatus::NotDownloaded, NodeErrorCode::NoError);
  case Status::DownloadFailed: return StatusAndError(NodeStatus::Error, NodeErrorCode::NoInetConnection);
  case Status::Downloading: return StatusAndError(NodeStatus::Downloading, NodeErrorCode::NoError);
  case Status::Applying: return StatusAndError(NodeStatus::Applying, NodeErrorCode::NoError);
  case Status::InQueue: return StatusAndError(NodeStatus::InQueue, NodeErrorCode::NoError);
  case Status::UnknownError: return StatusAndError(NodeStatus::Error, NodeErrorCode::UnknownError);
  case Status::OnDiskOutOfDate: return StatusAndError(NodeStatus::OnDiskOutOfDate, NodeErrorCode::NoError);
  case Status::OutOfMemFailed: return StatusAndError(NodeStatus::Error, NodeErrorCode::OutOfMemFailed);
  }
  UNREACHABLE();
}

std::string DebugPrint(StatusAndError statusAndError)
{
  std::ostringstream out;
  out << "StatusAndError[" << DebugPrint(statusAndError.status) << ", " << DebugPrint(statusAndError.error) << "]";
  return out.str();
}
}  // namespace storage

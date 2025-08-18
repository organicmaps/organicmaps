#include "storage/storage_defines.hpp"

#include "base/assert.hpp"

#include <sstream>

using namespace std;
using namespace string_literals;

namespace storage
{
storage::CountryId const kInvalidCountryId;

bool IsCountryIdValid(CountryId const & countryId)
{
  return countryId != kInvalidCountryId;
}

string DebugPrint(Status status)
{
  switch (status)
  {
  case Status::Undefined: return "EUndefined"s;
  case Status::OnDisk: return "OnDisk"s;
  case Status::NotDownloaded: return "NotDownloaded"s;
  case Status::DownloadFailed: return "DownloadFailed"s;
  case Status::Downloading: return "Downloading"s;
  case Status::Applying: return "Applying"s;
  case Status::InQueue: return "InQueue"s;
  case Status::UnknownError: return "Unknown"s;
  case Status::OnDiskOutOfDate: return "OnDiskOutOfDate"s;
  case Status::OutOfMemFailed: return "OutOfMemFailed"s;
  }
  UNREACHABLE();
}

string DebugPrint(NodeStatus status)
{
  switch (status)
  {
  case NodeStatus::Undefined: return "Undefined"s;
  case NodeStatus::Error: return "Error"s;
  case NodeStatus::OnDisk: return "OnDisk"s;
  case NodeStatus::NotDownloaded: return "NotDownloaded"s;
  case NodeStatus::Downloading: return "Downloading"s;
  case NodeStatus::Applying: return "Applying"s;
  case NodeStatus::InQueue: return "InQueue"s;
  case NodeStatus::OnDiskOutOfDate: return "OnDiskOutOfDate"s;
  case NodeStatus::Partly: return "Partly"s;
  }
  UNREACHABLE();
}

string DebugPrint(NodeErrorCode status)
{
  switch (status)
  {
  case NodeErrorCode::NoError: return "NoError"s;
  case NodeErrorCode::UnknownError: return "UnknownError"s;
  case NodeErrorCode::OutOfMemFailed: return "OutOfMemFailed"s;
  case NodeErrorCode::NoInetConnection: return "NoInetConnection"s;
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

string DebugPrint(StatusAndError statusAndError)
{
  ostringstream out;
  out << "StatusAndError[" << DebugPrint(statusAndError.status) << ", " << DebugPrint(statusAndError.error) << "]";
  return out.str();
}
}  // namespace storage

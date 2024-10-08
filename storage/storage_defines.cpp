#include "storage/storage_defines.hpp"

#include "base/assert.hpp"

#include <sstream>

using namespace std;
using namespace string_literals;

namespace storage
{
storage::CountryId const kInvalidCountryId;

bool IsCountryIdValid(CountryId const & countryId) { return countryId != kInvalidCountryId; }

StatusAndError ParseStatus(Status innerStatus)
{
  switch (innerStatus)
  {
  case Status::Undefined:
    return StatusAndError(NodeStatus::Undefined, NodeErrorCode::NoError);
  case Status::OnDisk:
    return StatusAndError(NodeStatus::OnDisk, NodeErrorCode::NoError);
  case Status::NotDownloaded:
    return StatusAndError(NodeStatus::NotDownloaded, NodeErrorCode::NoError);
  case Status::DownloadFailed:
    return StatusAndError(NodeStatus::Error, NodeErrorCode::NoInetConnection);
  case Status::Downloading:
    return StatusAndError(NodeStatus::Downloading, NodeErrorCode::NoError);
  case Status::Applying:
    return StatusAndError(NodeStatus::Applying, NodeErrorCode::NoError);
  case Status::InQueue:
    return StatusAndError(NodeStatus::InQueue, NodeErrorCode::NoError);
  case Status::UnknownError:
    return StatusAndError(NodeStatus::Error, NodeErrorCode::UnknownError);
  case Status::OnDiskOutOfDate:
    return StatusAndError(NodeStatus::OnDiskOutOfDate, NodeErrorCode::NoError);
  case Status::OutOfMemFailed:
    return StatusAndError(NodeStatus::Error, NodeErrorCode::OutOfMemFailed);
  }
  UNREACHABLE();
}

string DebugPrint(StatusAndError statusAndError)
{
  ostringstream out;
  out << "StatusAndError[" << ::DebugPrint(statusAndError.status)
      << ", " << ::DebugPrint(statusAndError.error) << "]";
  return out.str();
}
}  // namespace storage

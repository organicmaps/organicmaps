#include "map/nogps/motion_source.hpp"

#include "base/assert.hpp"

namespace nogps
{
std::string DebugPrint(SourceState state)
{
  switch (state)
  {
  case SourceState::Disconnected: return "DISCONNECTED";
  case SourceState::Connecting: return "CONNECTING";
  case SourceState::NoAdapter: return "NO_ADAPTER";
  case SourceState::ObdDisabled: return "OBD_DISABLED";
  case SourceState::ObdConnecting: return "OBD_CONNECTING";
  case SourceState::ObdError: return "OBD_ERROR";
  case SourceState::NoCarData: return "NO_CAR_DATA";
  case SourceState::BoxSleeping: return "BOX_SLEEPING";
  case SourceState::ObdSleeping: return "OBD_SLEEPING";
  case SourceState::Connected: return "CONNECTED";
  case SourceState::Updating: return "UPDATING";
  }
  UNREACHABLE();
}

std::string DebugPrint(CalibrationState state)
{
  switch (state)
  {
  case CalibrationState::None: return "NONE";
  case CalibrationState::Calibrating: return "CALIBRATING";
  case CalibrationState::Done: return "DONE";
  case CalibrationState::FailedMoving: return "FAILED_MOVING";
  case CalibrationState::MountMoved: return "MOUNT_MOVED";
  }
  UNREACHABLE();
}
}  // namespace nogps

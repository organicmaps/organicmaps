#include "map/nogps/delegate.hpp"

#include "base/assert.hpp"

namespace nogps
{
std::string DebugPrint(Provider provider)
{
  switch (provider)
  {
  case Provider::Gps: return "gps";
  case Provider::Network: return "network";
  case Provider::Fused: return "fused";
  case Provider::Manual: return "manual";
  case Provider::Inertial: return "inertial";
  }
  UNREACHABLE();
}

std::string DebugPrint(Event event)
{
  switch (event)
  {
  case Event::ManualModeChanged: return "ManualModeChanged";
  case Event::GpsBack: return "GpsBack";
  case Event::GpsLost: return "GpsLost";
  case Event::GpsSpoofed: return "GpsSpoofed";
  case Event::GpsRestored: return "GpsRestored";
  case Event::RoadLost: return "RoadLost";
  case Event::TurnsReversed: return "TurnsReversed";
  case Event::MotionSourceStopped: return "MotionSourceStopped";
  case Event::MarkNoRoad: return "MarkNoRoad";
  case Event::NotCalibrated: return "NotCalibrated";
  case Event::FirmwareUpdateAvailable: return "FirmwareUpdateAvailable";
  case Event::FirmwareUpdateDone: return "FirmwareUpdateDone";
  case Event::FirmwareUpdateFailed: return "FirmwareUpdateFailed";
  case Event::BoxCalibrationAdvised: return "BoxCalibrationAdvised";
  }
  UNREACHABLE();
}

std::string DebugPrint(BleState state)
{
  switch (state)
  {
  case BleState::Off: return "OFF";
  case BleState::NoPermission: return "NO_PERMISSION";
  case BleState::Disabled: return "DISABLED";
  case BleState::Searching: return "SEARCHING";
  case BleState::PairingClosed: return "PAIRING_CLOSED";
  case BleState::Connecting: return "CONNECTING";
  case BleState::Pairing: return "PAIRING";
  case BleState::Connected: return "CONNECTED";
  }
  UNREACHABLE();
}
}  // namespace nogps

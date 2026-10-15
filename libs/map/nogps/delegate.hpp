#pragma once

#include "geometry/latlon.hpp"

#include <cstdint>
#include <optional>
#include <string>

namespace nogps
{
enum class Provider
{
  // Satellites.
  Gps,
  // Cell towers and Wi-Fi.
  Network,
  // A mix of both, e.g. Google fused or CoreLocation: told apart by the accuracy.
  Fused,
  // Set by the user.
  Manual,
  // Calculated from the car speed and turns.
  Inertial,
};

/// A position from the platform or one to show.
struct Fix
{
  ms::LatLon m_position;
  double m_accuracyM = 0;
  // Clockwise from the north.
  std::optional<double> m_bearingDeg;
  std::optional<double> m_bearingAccuracyDeg;
  std::optional<double> m_speedMps;
  std::optional<double> m_altitudeM;
  std::optional<double> m_altitudeAccuracyM;
  // Monotonic, see Clock::NowMs().
  int64_t m_timeMs = 0;
  int64_t m_unixTimeMs = 0;
  Provider m_provider = Provider::Gps;
};

enum class Event
{
  ManualModeChanged,
  // The manual mode has been left by itself: GPS is trusted again.
  GpsBack,
  // The manual mode has been turned on by itself: GPS is lost or wrong while driving.
  GpsLost,
  GpsSpoofed,
  GpsRestored,
  // The inertial navigation has left the roads: the car is stopped on the road, the user should mark it.
  RoadLost,
  // The gyroscope turns the car the other way than GPS: the inertial navigation would turn it wrong.
  TurnsReversed,
  // The car speed has stopped coming while the car is followed without GPS. Repeated while it lasts.
  MotionSourceStopped,
  // The user has tapped the map in the manual mode where there is no road: the car is always on a road.
  MarkNoRoad,
  // The car drives without GPS, but its turns are not known: the gyroscope is not calibrated or the sensor box
  // has been moved. The position stands still. Repeated while it lasts.
  NotCalibrated,
  // The sensor box works with another firmware than the application comes with, and can take it now: the car
  // stands. Told once for a firmware of a box.
  FirmwareUpdateAvailable,
  // The update of the firmware of the sensor box has ended.
  FirmwareUpdateDone,
  FirmwareUpdateFailed,
  // The sensor box has been powered on: it is new in the car, or it was taken out and may be put in another way.
  // The user should calibrate it on a level place. Told once for a power-on, while the car stands.
  BoxCalibrationAdvised,
};

/// What the platform does about the ESP32 sensor box over Bluetooth LE, told to Engine::OnEsp32BleState().
enum class BleState
{
  // Bluetooth is not searched: it is not asked for, or the platform has no Bluetooth LE.
  Off,
  // The user has not allowed the app to use Bluetooth.
  NoPermission,
  // Bluetooth is switched off on the phone.
  Disabled,
  Searching,
  // A box is found, but it doesn't know the phone and doesn't accept a new one now: it does for two minutes
  // after it is powered.
  PairingClosed,
  Connecting,
  // The phone asks the user for the code of the box.
  Pairing,
  // Connected and subscribed to the lines of the box.
  Connected,
};

std::string DebugPrint(Provider provider);
std::string DebugPrint(Event event);
std::string DebugPrint(BleState state);

/// What the platform does for the navigation without GPS. Everything is called on the GUI thread, and the
/// platform calls the Engine back on it.
class Delegate
{
public:
  virtual ~Delegate() = default;

  /// The accelerometer of the phone, and the gyroscope if |gyroscope|, to Engine::OnAccel() and OnGyro().
  virtual void StartMotionSensors(bool gyroscope) = 0;
  virtual void StopMotionSensors() = 0;

  /// The ELM327 adapter over Bluetooth SPP: Engine::OnElm327Connected() or OnElm327Closed() follows.
  virtual void Elm327Connect(std::string const & address) = 0;
  virtual void Elm327Write(std::string const & data) = 0;
  virtual void Elm327Close() = 0;

  /// The ESP32 sensor box over Bluetooth LE, the only link to it. The platform searches the box by its service,
  /// connects, subscribes to the characteristic with its lines and keeps doing it until closed: the bytes go
  /// to Engine::OnEsp32BleBytes() as they come, every change of the state to Engine::OnEsp32BleState(). The
  /// UUIDs and the pairing are in docs/nogps/esp32-firmware-spec.md, section 17. A platform that hasn't made
  /// it yet doesn't get the box.
  virtual void Esp32BleOpen() {}
  virtual void Esp32BleSend(std::string const & /* line */) {}
  /// A piece of a new firmware for the box: written to its characteristic for them without a response, in the
  /// order of the calls, the lines of Esp32BleSend() go between the pieces. Section 18 of the same document.
  virtual void Esp32BleSendFirmware(std::string const & /* piece */) {}
  virtual void Esp32BleClose() {}

  /// The position to show and to navigate by.
  virtual void OnPosition(Fix const & fix) = 0;
  virtual void OnEvent(Event event) = 0;
};
}  // namespace nogps

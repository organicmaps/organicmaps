#pragma once

#include <array>
#include <cstdint>
#include <optional>
#include <string>

namespace nogps
{
/// What the sensor box knows about the car besides its speed. The box asks the car for the engine speed and
/// ELM327 for the voltage only while the car stands, not to delay the speed. -1 is unknown.
struct CarInfo
{
  /// \returns the voltage the box compares its own one with: of the control unit of the car, ELM327 clones
  /// show less. -1 if unknown.
  int GetReferenceMillivolts() const { return m_ecuMillivolts >= 0 ? m_ecuMillivolts : m_elmMillivolts; }

  std::optional<bool> m_engineRunning;
  int m_rpm = -1;
  // The voltage of the car in millivolts. The box measures it by itself all the time, ELM327 and the control unit
  // of the car are asked while the car stands.
  int m_boxMillivolts = -1;
  int m_elmMillivolts = -1;
  int m_ecuMillivolts = -1;
  // The voltage of the box differs too much from the car: the voltage divider of the box is wrong.
  bool m_voltageMismatch = false;
};

enum class SourceState
{
  Disconnected,
  Connecting,
  // The ELM327 adapter doesn't answer: it is not plugged in, switched off or broken.
  NoAdapter,
  // The sensor box is set up not to use ELM327.
  ObdDisabled,
  // The sensor box is connected, its ELM327 is being initialized or searches the protocol of the car.
  ObdConnecting,
  // ELM327 of the sensor box has a bus error or has stopped answering.
  ObdError,
  // Connected, but the car doesn't tell its speed (ignition off, protocol search).
  NoCarData,
  // The sensor box has gone after the engine was stopped: it switches its radio off to save the car battery.
  BoxSleeping,
  // The sensor box is connected and doesn't talk to the car with the engine stopped.
  ObdSleeping,
  Connected,
  // The sensor box takes a new firmware: it sends no data until it restarts with it.
  Updating,
};

enum class CalibrationState
{
  None,
  Calibrating,
  Done,
  // The phone or the car moved during the calibration.
  FailedMoving,
  // The sensor box has been moved in the car since its calibration.
  MountMoved,
};

std::string DebugPrint(SourceState state);
std::string DebugPrint(CalibrationState state);

/// Where the inertial navigation takes the car movement from: the speed of the car and its rotation.
class MotionSource
{
public:
  class Listener
  {
  public:
    virtual ~Listener() = default;

    /// \param timeMs monotonic time the speed was measured at.
    virtual void OnSpeed(int speedKmh, int64_t timeMs) = 0;

    /// \param yawDeltaDeg the clockwise rotation of the car during |dtSec|, 0 while the source is not calibrated.
    /// \param timestampNs monotonic time of the end of the interval.
    virtual void OnMotion(double yawDeltaDeg, double dtSec, int64_t timestampNs) = 0;

    /// \param accel the acceleration of the source itself along its three fixed axes, m/s², if it has an
    /// accelerometer fixed in the car.
    virtual void OnSourceAccel(std::array<double, 3> const & /* accel */) {}
  };

  virtual ~MotionSource() = default;

  virtual void Start() = 0;
  virtual void Stop() = 0;

  virtual SourceState GetState() const = 0;

  /// \returns true if the rotation of the car is known: the gyroscope is zeroed and its position in the car is
  /// known.
  virtual bool IsCalibrated() const = 0;
  virtual CalibrationState GetCalibrationState() = 0;
  virtual int GetCalibrationProgressPercent() const = 0;

  /// Zeroes the gyroscope and finds where it looks in the car. The car must stand still for ~3 s.
  virtual void Calibrate() = 0;

  /// \returns the name of the device to show, empty for the phone.
  virtual std::string GetDeviceName() const = 0;

  /// \returns nothing if the source doesn't tell it: the phone, or the box that is not connected.
  virtual std::optional<CarInfo> GetCarInfo() const { return {}; }
};
}  // namespace nogps

#pragma once

#include "map/nogps/accel_log.hpp"
#include "map/nogps/esp32_protocol.hpp"
#include "map/nogps/esp32_update.hpp"
#include "map/nogps/motion_source.hpp"
#include "map/nogps/scheduler.hpp"

#include <cstdint>
#include <deque>
#include <optional>
#include <set>
#include <string>
#include <string_view>

namespace nogps
{
class Clock;
class Delegate;

/// The NoGPS ESP32 sensor box: it is fixed in the car and sends the car rotation and speed over Bluetooth LE,
/// the phone can be held in hands. The box zeroes its gyroscope by itself on every stop.
class Esp32Source
  : public MotionSource
  , private Esp32Update::Link
{
public:
  // The box sends data while it hears the phone.
  static int64_t constexpr kHelloIntervalMs = 1000;
  // The box sends 50 lines a second, a longer silence means it is lost.
  static int64_t constexpr kDataTimeoutMs = 1000;
  // The seconds of the accelerometer written to the trip log from before the car has reported that it moves.
  static size_t constexpr kAccelLinesBeforeStart = 3;
  // A calibration takes ~3 s on the box.
  static int64_t constexpr kCalibrationTimeoutMs = 10'000;
  // The events missed are asked again not more often than this.
  static int64_t constexpr kEventsRequestIntervalMs = 2000;
  // More than the box keeps in its journal.
  static size_t constexpr kMaxLoggedEvents = 64;
  // The time of the box is 32 bits of milliseconds and wraps around.
  static int64_t constexpr kBoxTimeWrapMs = 1LL << 32;
  // The lines of the box are shorter.
  static size_t constexpr kMaxLineSize = 512;
  // A box that has not told about itself is asked after this many hellos, this many times: an old firmware
  // doesn't know the question.
  static int constexpr kInfoRequestHellos = 3;
  static int constexpr kMaxInfoRequests = 3;

  /// A start of the box by a power-on: it was plugged into the car.
  struct PowerOn
  {
    // Milliseconds since the Unix epoch.
    int64_t m_unixMs = 0;
    // How long the box has worked since then by its own clock.
    int64_t m_uptimeMs = 0;
  };

  /// How the data lines came during the last second, for the log of a drive.
  struct DataRate
  {
    int m_lines = 0;
    // The longest pause between two lines, or since the last one.
    int64_t m_maxGapMs = 0;
  };

  Esp32Source(Delegate & delegate, Scheduler & scheduler, Clock const & clock, MotionSource::Listener & listener);
  ~Esp32Source() override;

  // MotionSource overrides:
  void Start() override;
  void Stop() override;
  SourceState GetState() const override;
  bool IsCalibrated() const override;
  CalibrationState GetCalibrationState() override;
  int GetCalibrationProgressPercent() const override { return m_calibrationProgress; }
  void Calibrate() override;
  std::string GetDeviceName() const override;
  std::optional<CarInfo> GetCarInfo() const override;

  /// The bytes of the box as they come over Bluetooth: a line may be cut in pieces, a piece may have several lines.
  void OnBleBytes(std::string_view bytes);
  void OnBleState(BleState state);
  BleState GetBleState() const { return m_bleState; }
  DataRate GetDataRate() const;

  /// \returns when the box was powered on, if this is how it has started and its data come. Nothing after a
  /// restart of another kind: the box has stayed in its place.
  std::optional<PowerOn> GetPowerOn() const;
  /// \returns when the box was calibrated by Calibrate() the last time, nothing if it was not.
  std::optional<int64_t> GetCalibratedMs() const { return m_calibratedMs; }

  /// What the box has told about itself, nothing until it does.
  std::optional<esp32::Info> const & GetInfo() const { return m_info; }
  /// \returns the flags of the last data line of the box, see esp32::kFlag*.
  int GetFlags() const { return m_flags; }
  /// \returns true if the box takes a firmware now: it is connected over Bluetooth and its firmware knows how.
  bool CanUpdate() const;
  void StartUpdate(Firmware const & firmware);
  void CancelUpdate() { m_update.Cancel(); }
  Esp32Update const & GetUpdate() const { return m_update; }

private:
  // Esp32Update::Link overrides:
  int SendUpdateCommand(std::string const & command) override;
  void SendFirmwarePiece(std::string const & piece) override;

  void OnLine(std::string_view text);
  void SendHello();
  void Send(std::string_view command, int id);
  void OnData(esp32::Data const & data);
  void OnEvent(esp32::Event const & event);
  void OnReply(esp32::Reply const & reply);
  void OnInfo(esp32::Info const & info);
  void OnLastEventNumber(int64_t lastNumber);
  void RequestLostEvents(int64_t fromNumber);
  void ForgetJournal();
  bool IsConnected() const;
  bool IsSleeping() const;

  Delegate & m_delegate;
  Clock const & m_clock;
  MotionSource::Listener & m_listener;
  Timer m_helloTimer;
  bool m_running = false;
  int m_helloId = 0;

  BleState m_bleState = BleState::Off;
  // The line being received over Bluetooth, from its "$" on.
  std::string m_bleLine;
  // When the data lines of the last second came.
  std::deque<int64_t> m_dataTimesMs;

  // Nothing until the first data line.
  std::optional<int64_t> m_lastDataMs;
  int m_flags = 0;
  AccelLog m_accelLog;
  // The lines of the accelerometer while the car reports that it stands.
  std::deque<std::string> m_accelLinesBeforeStart;
  bool m_hasLast = false;
  int64_t m_lastTimeMs = 0;
  int64_t m_lastYawMdeg = 0;
  std::string m_imuName;
  // The state of the ELM327 adapter of the box from its status.
  std::optional<std::string> m_obdState;
  // The box hasn't found its ELM327: while it searches it again, the adapter is still missing.
  bool m_obdAdapterMissing = false;
  // The engine, its speed and the voltage of the car from the status of the box.
  std::optional<CarInfo> m_carInfo;
  // The box has told that its voltage and the one of ELM327 differ. Kept until they come close again.
  bool m_voltageMismatch = false;
  // The numbers of the events of the box written to the log.
  std::set<int64_t> m_loggedEvents;
  std::optional<int64_t> m_eventsRequestMs;
  // The journal of the box tells it has started by a power-on.
  bool m_poweredOn = false;
  int m_nextCommandId = 1;
  // The calibration command waiting for its reply, 0 if none.
  int m_calibrationId = 0;
  int64_t m_calibrationStartMs = 0;
  int m_calibrationProgress = 0;
  bool m_calibrationFailed = false;
  std::optional<int64_t> m_calibratedMs;

  std::optional<esp32::Info> m_info;
  int m_hellosWithoutInfo = 0;
  Esp32Update m_update;
};
}  // namespace nogps

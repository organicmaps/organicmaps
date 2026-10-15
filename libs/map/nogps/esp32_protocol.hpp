#pragma once

#include "map/nogps/motion_source.hpp"

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace nogps
{
/// Lines of the NoGPS ESP32 sensor box protocol, version 1: "$NGD,...*CRC\r\n". The checksum is
/// CRC-16/CCITT-FALSE of the bytes between '$' and '*'. See docs/nogps/esp32-firmware-spec.md.
namespace esp32
{
int constexpr kVersion = 1;

// Bits of the data flags.
int constexpr kFlagImuOk = 1;
int constexpr kFlagBiasOk = 1 << 1;
int constexpr kFlagUpOk = 1 << 2;
int constexpr kFlagFwdOk = 1 << 3;
int constexpr kFlagObdOk = 1 << 4;
int constexpr kFlagStill = 1 << 5;
int constexpr kFlagCalibrating = 1 << 6;
int constexpr kFlagMountMoved = 1 << 7;
int constexpr kFlagReverse = 1 << 8;
int constexpr kFlagObdAbsent = 1 << 9;
int constexpr kFlagError = 1 << 10;
// The box tells about a wrong voltage divider from the same difference.
int constexpr kMaxVoltageDiffPercent = 10;

using Fields = std::vector<std::string>;

/// A data line: the totals since the box has started, so a lost line loses nothing.
struct Data
{
  int64_t m_seq = 0;
  int64_t m_timeMs = 0;
  // Clockwise, thousandths of a degree.
  int64_t m_yawMdeg = 0;
  int64_t m_rateMdps = 0;
  int64_t m_distMm = 0;
  // -1 if the car doesn't tell it.
  int m_speedKmh = -1;
  int m_speedAgeMs = 0;
  int m_flags = 0;
  // The acceleration of the box during the line, m/s²: along two horizontal axes and up without the gravity,
  // and the largest deviation of the vertical one. Not sent by older boxes and before the calibration.
  bool m_hasAccel = false;
  double m_accelH1 = 0;
  double m_accelH2 = 0;
  double m_accelUp = 0;
  double m_jolt = 0;
};

/// An event of the box: an error, its end or a calibration.
struct Event
{
  int64_t m_number = 0;
  int64_t m_timeMs = 0;
  // E - error, W - warning, I - information.
  std::string m_level;
  std::string m_code;
  std::string m_text;
};

/// A reply to a command.
struct Reply
{
  int m_id = 0;
  bool m_ok = false;
  // The progress in percent, nothing if it is the final reply.
  std::optional<int> m_progress;
  std::string m_error;
  // What an accepted command returns, e.g. the size of a piece and of the window of a firmware update.
  std::vector<std::string> m_values;
};

/// What the box tells about itself when a connection starts.
struct Info
{
  std::string m_firmware;
  // A firmware is made for a chip and a board.
  std::string m_chip;
  std::string m_board;
  // The box takes a new firmware over Bluetooth.
  bool m_canUpdate = false;
  // The four characters of the name of the box, NoGPS-XXXX.
  std::string m_id;
  // How many times the box was powered on, nothing from a firmware that doesn't count it.
  std::optional<int64_t> m_powerOns;
};

/// The firmware the box is receiving.
struct UpdateProgress
{
  // The bytes written in a row from the start of the firmware.
  uint32_t m_received = 0;
  // RECV, RESEND, VERIFY, DONE, ABORTED or ERR_<code>.
  std::string m_state;
};

uint16_t Crc16(std::string_view text);

/// \returns the command line to send to the box.
std::string Command(int id, std::string_view command);

/// \returns the fields of a line with a valid checksum, the first one is the line type.
std::optional<Fields> Parse(std::string_view line);

/// \returns the data of an NGD line.
std::optional<Data> ParseData(Fields const & fields);
/// \returns the reply of an NGA line.
std::optional<Reply> ParseReply(Fields const & fields);
/// \returns the event of an NGE line.
std::optional<Event> ParseEvent(Fields const & fields);
/// \returns what an NGI line tells about the box.
std::optional<Info> ParseInfo(Fields const & fields);
/// \returns the progress of a firmware update from an NGO line.
std::optional<UpdateProgress> ParseUpdateProgress(Fields const & fields);
/// \returns the number of the last event of the box from an NGS line, if the box tells it.
std::optional<int64_t> ParseLastEventNumber(Fields const & fields);
/// \returns the state of the OBD adapter from an NGS line: NO_ADAPTER, NO_CAR, OK etc.
std::optional<std::string> ParseObdState(Fields const & fields);
/// \returns the name of the gyroscope from an NGS line.
std::optional<std::string> ParseImuName(Fields const & fields);
/// \returns what the box tells about the car in its status.
std::optional<CarInfo> ParseCarInfo(Fields const & fields);
}  // namespace esp32
}  // namespace nogps

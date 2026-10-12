#include "map/nogps/esp32_protocol.hpp"

#include "base/string_utils.hpp"

#include <cstdio>
#include <cstdlib>

namespace nogps
{
namespace esp32
{
namespace
{
std::string_view TrimLine(std::string_view s)
{
  while (!s.empty() && static_cast<unsigned char>(s.front()) <= ' ')
    s.remove_prefix(1);
  while (!s.empty() && static_cast<unsigned char>(s.back()) <= ' ')
    s.remove_suffix(1);
  return s;
}

// Splits by commas keeping the empty fields: the optional fields of the status may be empty.
Fields SplitFields(std::string_view body)
{
  Fields fields;
  while (true)
  {
    auto const comma = body.find(',');
    fields.emplace_back(body.substr(0, comma));
    if (comma == std::string_view::npos)
      return fields;
    body.remove_prefix(comma + 1);
  }
}

bool IsLine(Fields const & fields, size_t minSize, std::string_view type)
{
  return fields.size() >= minSize && fields[0] == type;
}

// Returns -1 for an absent or an empty field.
int ParseOptionalInt(Fields const & fields, size_t index)
{
  int value;
  if (index >= fields.size() || !strings::to_int(fields[index], value))
    return -1;
  return value;
}
}  // namespace

uint16_t Crc16(std::string_view text)
{
  uint16_t crc = 0xFFFF;
  for (char const c : text)
  {
    crc ^= static_cast<uint16_t>(static_cast<unsigned char>(c) << 8);
    for (int i = 0; i < 8; ++i)
      crc = (crc & 0x8000) ? static_cast<uint16_t>((crc << 1) ^ 0x1021) : static_cast<uint16_t>(crc << 1);
  }
  return crc;
}

std::string Command(int id, std::string_view command)
{
  std::string const body = "NGC," + std::to_string(id) + "," + std::string(command);
  char crc[8];
  std::snprintf(crc, sizeof(crc), "%04X", Crc16(body));
  return "$" + body + "*" + crc + "\r\n";
}

std::optional<Fields> Parse(std::string_view line)
{
  auto const trimmed = TrimLine(line);
  auto const star = trimmed.rfind('*');
  if (trimmed.empty() || trimmed.front() != '$' || star == std::string_view::npos || trimmed.size() != star + 5)
    return {};
  auto const body = trimmed.substr(1, star - 1);
  uint32_t crc;
  if (!strings::to_uint(trimmed.substr(star + 1), crc, 16) || crc != Crc16(body))
    return {};
  return SplitFields(body);
}

std::optional<Data> ParseData(Fields const & fields)
{
  int version;
  if (!IsLine(fields, 10, "NGD") || !strings::to_int(fields[1], version) || version != kVersion)
    return {};
  Data data;
  if (!strings::to_int(fields[2], data.m_seq) || !strings::to_int(fields[3], data.m_timeMs) ||
      !strings::to_int(fields[4], data.m_yawMdeg) || !strings::to_int(fields[5], data.m_rateMdps) ||
      !strings::to_int(fields[6], data.m_distMm) || !strings::to_int(fields[7], data.m_speedKmh) ||
      !strings::to_int(fields[8], data.m_speedAgeMs) || !strings::to_int(fields[9], data.m_flags, 16))
  {
    return {};
  }
  if (fields.size() >= 14 && !fields[10].empty())
  {
    int h1, h2, up, jolt;
    if (!strings::to_int(fields[10], h1) || !strings::to_int(fields[11], h2) || !strings::to_int(fields[12], up) ||
        !strings::to_int(fields[13], jolt))
    {
      return {};
    }
    data.m_accelH1 = h1 / 1000.0;
    data.m_accelH2 = h2 / 1000.0;
    data.m_accelUp = up / 1000.0;
    data.m_jolt = jolt / 1000.0;
    data.m_hasAccel = true;
  }
  return data;
}

std::optional<Reply> ParseReply(Fields const & fields)
{
  Reply reply;
  if (!IsLine(fields, 3, "NGA") || !strings::to_int(fields[1], reply.m_id))
    return {};
  if (fields[2] == "OK")
  {
    reply.m_ok = true;
    reply.m_values.assign(fields.begin() + 3, fields.end());
  }
  else if (fields[2] == "PROGRESS")
  {
    int progress = 0;
    if (fields.size() > 3 && !strings::to_int(fields[3], progress))
      return {};
    reply.m_progress = progress;
  }
  else if (fields[2] == "ERR")
  {
    if (fields.size() > 3)
      reply.m_error = fields[3];
  }
  else
  {
    return {};
  }
  return reply;
}

std::optional<Event> ParseEvent(Fields const & fields)
{
  Event event;
  if (!IsLine(fields, 7, "NGE") || !strings::to_int(fields[2], event.m_number) ||
      !strings::to_int(fields[3], event.m_timeMs))
  {
    return {};
  }
  event.m_level = fields[4];
  event.m_code = fields[5];
  event.m_text = fields[6];
  return event;
}

std::optional<Info> ParseInfo(Fields const & fields)
{
  // Version, firmware, build date, chip, board, id, flash size, largest image, partition, its state, abilities,
  // power-ons.
  int version;
  if (!IsLine(fields, 6, "NGI") || !strings::to_int(fields[1], version) || version != kVersion)
    return {};
  Info info;
  info.m_firmware = fields[2];
  info.m_chip = fields[4];
  info.m_board = fields[5];
  if (fields.size() > 11)
    for (auto const ability : strings::Tokenize(fields[11], " "))
      if (ability == "OTA")
        info.m_canUpdate = true;
  if (fields.size() > 6)
    info.m_id = fields[6];
  if (int64_t powerOns; fields.size() > 12 && strings::to_int(fields[12], powerOns))
    info.m_powerOns = powerOns;
  return info;
}

std::optional<UpdateProgress> ParseUpdateProgress(Fields const & fields)
{
  int version;
  UpdateProgress progress;
  if (!IsLine(fields, 4, "NGO") || !strings::to_int(fields[1], version) || version != kVersion ||
      !strings::to_uint(fields[2], progress.m_received))
  {
    return {};
  }
  progress.m_state = fields[3];
  return progress;
}

std::optional<int64_t> ParseLastEventNumber(Fields const & fields)
{
  int64_t number;
  if (!IsLine(fields, 16, "NGS") || !strings::to_int(fields[15], number))
    return {};
  return number;
}

std::optional<std::string> ParseObdState(Fields const & fields)
{
  if (!IsLine(fields, 6, "NGS"))
    return {};
  return fields[5];
}

std::optional<std::string> ParseImuName(Fields const & fields)
{
  if (!IsLine(fields, 4, "NGS"))
    return {};
  return fields[3];
}

std::optional<CarInfo> ParseCarInfo(Fields const & fields)
{
  // Fields 17-21 of the status: engine, rpm, the voltage measured by the box, by ELM327 and by the control
  // unit of the car. Older firmwares send less fields.
  if (!IsLine(fields, 17, "NGS"))
    return {};
  CarInfo car;
  if (fields[16] == "RUN")
    car.m_engineRunning = true;
  else if (fields[16] == "OFF")
    car.m_engineRunning = false;
  car.m_rpm = ParseOptionalInt(fields, 17);
  car.m_boxMillivolts = ParseOptionalInt(fields, 18);
  car.m_elmMillivolts = ParseOptionalInt(fields, 19);
  car.m_ecuMillivolts = ParseOptionalInt(fields, 20);
  int const reference = car.GetReferenceMillivolts();
  car.m_voltageMismatch = car.m_boxMillivolts >= 0 && reference >= 0 &&
                          std::abs(car.m_boxMillivolts - reference) * 100 > reference * kMaxVoltageDiffPercent;
  return car;
}
}  // namespace esp32
}  // namespace nogps

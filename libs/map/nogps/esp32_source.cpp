#include "map/nogps/esp32_source.hpp"

#include "map/nogps/clock.hpp"
#include "map/nogps/delegate.hpp"

#include "base/logging.hpp"

#include <algorithm>

namespace nogps
{
Esp32Source::Esp32Source(Delegate & delegate, Scheduler & scheduler, Clock const & clock,
                         MotionSource::Listener & listener)
  : m_delegate(delegate)
  , m_clock(clock)
  , m_listener(listener)
  , m_helloTimer(scheduler)
  , m_update(*this, scheduler, clock)
{}

Esp32Source::~Esp32Source()
{
  Stop();
}

void Esp32Source::Start()
{
  if (m_running)
    return;
  LOG(LINFO, ("ESP32 started"));
  m_running = true;
  m_delegate.Esp32BleOpen();
  SendHello();
}

void Esp32Source::Stop()
{
  if (!m_running)
    return;
  LOG(LINFO, ("ESP32 stopped"));
  m_running = false;
  m_helloTimer.Stop();
  m_delegate.Esp32BleClose();
  m_bleState = BleState::Off;
  m_bleLine.clear();
  m_dataTimesMs.clear();
  m_hasLast = false;
  m_lastDataMs.reset();
  m_info.reset();
  m_hellosWithoutInfo = 0;
  m_update.Reset();
  ForgetJournal();
}

void Esp32Source::SendHello()
{
  Send("HELLO", ++m_helloId);
  // The box tells about itself when a connection starts, the application may have missed it: the box was
  // talking to it before it was restarted.
  if (!m_info && IsConnected() && m_hellosWithoutInfo < kInfoRequestHellos * kMaxInfoRequests &&
      ++m_hellosWithoutInfo % kInfoRequestHellos == 0)
  {
    Send("INFO", m_nextCommandId++);
  }
  m_helloTimer.Start(kHelloIntervalMs, [this] { SendHello(); });
}

int Esp32Source::SendUpdateCommand(std::string const & command)
{
  int const id = m_nextCommandId++;
  Send(command, id);
  return id;
}

void Esp32Source::SendFirmwarePiece(std::string const & piece)
{
  if (m_bleState == BleState::Connected)
    m_delegate.Esp32BleSendFirmware(piece);
}

bool Esp32Source::CanUpdate() const
{
  return m_running && m_bleState == BleState::Connected && m_info && m_info->m_canUpdate && !m_update.IsActive();
}

void Esp32Source::StartUpdate(Firmware const & firmware)
{
  if (CanUpdate())
    m_update.Start(firmware.m_read(), firmware.m_version);
}

void Esp32Source::OnInfo(esp32::Info const & info)
{
  if (!m_info || m_info->m_firmware != info.m_firmware)
    LOG(LINFO, ("Box", info.m_id, "firmware =", info.m_firmware, info.m_chip, info.m_board, "updates =",
               info.m_canUpdate));
  m_info = info;
  m_update.OnBoxFirmware(info.m_firmware);
}

void Esp32Source::Send(std::string_view command, int id)
{
  if (m_bleState == BleState::Connected)
    m_delegate.Esp32BleSend(esp32::Command(id, command));
}

void Esp32Source::OnBleState(BleState state)
{
  if (!m_running || state == m_bleState)
    return;
  LOG(LINFO, ("Bluetooth =", state));
  m_bleState = state;
  m_bleLine.clear();
  if (state != BleState::Connected)
  {
    // Another box may be found, or this one with a new firmware.
    m_info.reset();
    m_hellosWithoutInfo = 0;
    m_update.OnLinkLost();
    ForgetJournal();
  }
  // The box starts sending at once, not at the next HELLO.
  if (state == BleState::Connected)
    SendHello();
}

void Esp32Source::OnBleBytes(std::string_view bytes)
{
  if (!m_running)
    return;
  for (char const c : bytes)
  {
    // A line starts with "$": what was received of a line cut in the middle is dropped.
    if (c == '$')
      m_bleLine.clear();
    else if (m_bleLine.empty())
      continue;
    if (c == '\n')
    {
      OnLine(m_bleLine);
      m_bleLine.clear();
    }
    else if (m_bleLine.size() < kMaxLineSize)
    {
      m_bleLine += c;
    }
    else
    {
      m_bleLine.clear();
    }
  }
}

Esp32Source::DataRate Esp32Source::GetDataRate() const
{
  int64_t const now = m_clock.NowMs();
  DataRate rate;
  int64_t previousMs = now - kDataTimeoutMs;
  for (int64_t const timeMs : m_dataTimesMs)
  {
    if (timeMs <= now - kDataTimeoutMs)
      continue;
    ++rate.m_lines;
    rate.m_maxGapMs = std::max(rate.m_maxGapMs, timeMs - previousMs);
    previousMs = timeMs;
  }
  rate.m_maxGapMs = std::max(rate.m_maxGapMs, now - previousMs);
  return rate;
}

void Esp32Source::OnLine(std::string_view text)
{
  auto const fields = esp32::Parse(text);
  if (!fields)
    return;
  if (auto const data = esp32::ParseData(*fields))
  {
    OnData(*data);
    return;
  }
  if (auto const reply = esp32::ParseReply(*fields))
  {
    OnReply(*reply);
    return;
  }
  if (auto const event = esp32::ParseEvent(*fields))
  {
    OnEvent(*event);
    return;
  }
  if (auto const info = esp32::ParseInfo(*fields))
  {
    OnInfo(*info);
    return;
  }
  if (auto const progress = esp32::ParseUpdateProgress(*fields))
  {
    m_update.OnProgress(*progress);
    return;
  }
  if (auto const lastNumber = esp32::ParseLastEventNumber(*fields))
    OnLastEventNumber(*lastNumber);
  if (auto const imuName = esp32::ParseImuName(*fields))
    m_imuName = *imuName;
  if (auto const carInfo = esp32::ParseCarInfo(*fields))
  {
    if (carInfo->m_boxMillivolts >= 0 && carInfo->GetReferenceMillivolts() >= 0)
      m_voltageMismatch = carInfo->m_voltageMismatch;
    m_carInfo = carInfo;
  }
  auto const obdState = esp32::ParseObdState(*fields);
  if (obdState && obdState != m_obdState)
  {
    LOG(LINFO, ("OBD state =", *obdState));
    m_obdState = obdState;
    if (*obdState == "NO_ADAPTER")
      m_obdAdapterMissing = true;
    else if (*obdState != "INIT")
      m_obdAdapterMissing = false;
  }
}

void Esp32Source::OnData(esp32::Data const & data)
{
  int64_t const now = m_clock.NowMs();
  bool const fresh = m_lastDataMs && now - *m_lastDataMs <= kDataTimeoutMs;
  m_lastDataMs = now;
  m_dataTimesMs.push_back(now);
  while (now - m_dataTimesMs.front() >= kDataTimeoutMs)
    m_dataTimesMs.pop_front();
  m_flags = data.m_flags;
  if (data.m_speedKmh >= 0)
    m_listener.OnSpeed(data.m_speedKmh, now - data.m_speedAgeMs);
  // For the trip log only, see AccelLog.
  if (data.m_hasAccel)
  {
    auto const line =
        m_accelLog.OnSample(data.m_timeMs * 1'000'000, data.m_accelH1, data.m_accelH2, data.m_accelUp, data.m_jolt);
    // What the sensor has felt before a break is not before the start.
    if (!fresh)
      m_accelLinesBeforeStart.clear();
    if (line && data.m_speedKmh > 0)
    {
      // The car reports that it moves a second late. What the sensor has felt before that tells which way the car
      // has started, forward or backward.
      for (size_t i = 0; i < m_accelLinesBeforeStart.size(); ++i)
        LOG(LINFO, ("ACCB_BEFORE", m_accelLinesBeforeStart.size() - i, m_accelLinesBeforeStart[i]));
      m_accelLinesBeforeStart.clear();
      LOG(LINFO, ("ACCB", *line));
    }
    else if (line)
    {
      m_accelLinesBeforeStart.push_back(std::move(*line));
      if (m_accelLinesBeforeStart.size() > kAccelLinesBeforeStart)
        m_accelLinesBeforeStart.pop_front();
    }
    m_listener.OnSourceAccel({data.m_accelH1, data.m_accelH2, data.m_accelUp});
  }

  // The time of the box has gone back, and not because its 32 bits have wrapped around: the box has restarted
  // and counts its rotation and its events from the start.
  if (m_hasLast && data.m_timeMs < m_lastTimeMs && m_lastTimeMs - data.m_timeMs < kBoxTimeWrapMs / 2)
  {
    LOG(LINFO, ("The box has restarted"));
    ForgetJournal();
    m_hasLast = false;
  }

  // The box sends the totals: a lost line loses nothing, the next one has the rotation.
  if (m_hasLast && fresh)
  {
    int64_t const dtMs = (data.m_timeMs - m_lastTimeMs) & (kBoxTimeWrapMs - 1);
    double const yawDeltaDeg = IsCalibrated() ? (data.m_yawMdeg - m_lastYawMdeg) / 1000.0 : 0;
    m_listener.OnMotion(yawDeltaDeg, dtMs / 1000.0, now * 1'000'000);
  }
  m_hasLast = true;
  m_lastTimeMs = data.m_timeMs;
  m_lastYawMdeg = data.m_yawMdeg;
}

void Esp32Source::OnEvent(esp32::Event const & event)
{
  int64_t const lastNumber = m_loggedEvents.empty() ? -1 : *m_loggedEvents.rbegin();
  // The journal of the box sent on request comes after the new events, so the order doesn't matter.
  if (!m_loggedEvents.insert(event.m_number).second)
    return;
  if (m_loggedEvents.size() > kMaxLoggedEvents)
    m_loggedEvents.erase(m_loggedEvents.begin());
  // The errors of the box are logged to find later why the speed or the gyroscope was lost.
  if (event.m_level == "I")
    LOG(LINFO, ("Box event #", event.m_number, "at", event.m_timeMs, "ms:", event.m_code, event.m_text));
  else
    LOG(LWARNING, ("Box event #", event.m_number, "at", event.m_timeMs, "ms:", event.m_code, event.m_text));
  // The box compares the raw voltage, the status has the corrected one.
  if (event.m_code == "VOLT_MISMATCH")
    m_voltageMismatch = true;
  else if (event.m_code == "VOLT_CAL")
    m_voltageMismatch = false;
  // "fw 0.3.3 reset POWERON": the box was plugged in. It restarts by itself for other reasons.
  else if (event.m_code == "BOOT")
    m_poweredOn = event.m_text.ends_with("reset POWERON");
  if (event.m_number > lastNumber + 1)
    RequestLostEvents(lastNumber + 1);
}

void Esp32Source::RequestLostEvents(int64_t fromNumber)
{
  // The events were lost or came before the connection, the journal of the box has them.
  int64_t const now = m_clock.NowMs();
  if (m_eventsRequestMs && now - *m_eventsRequestMs < kEventsRequestIntervalMs)
    return;
  m_eventsRequestMs = now;
  Send("EVENTS," + std::to_string(fromNumber), m_nextCommandId++);
}

void Esp32Source::ForgetJournal()
{
  // The journal is asked from its start again.
  m_loggedEvents.clear();
  m_poweredOn = false;
}

void Esp32Source::OnLastEventNumber(int64_t lastNumber)
{
  if (lastNumber < 0)
    return;
  // The box has restarted and counts its events again.
  if (!m_loggedEvents.empty() && lastNumber < *m_loggedEvents.rbegin())
    ForgetJournal();
  int64_t const loggedNumber = m_loggedEvents.empty() ? -1 : *m_loggedEvents.rbegin();
  if (lastNumber > loggedNumber)
    RequestLostEvents(loggedNumber + 1);
}

void Esp32Source::OnReply(esp32::Reply const & reply)
{
  if (m_update.OnReply(reply) || reply.m_id != m_calibrationId)
    return;
  if (reply.m_progress)
  {
    m_calibrationProgress = *reply.m_progress;
    return;
  }
  LOG(LINFO, ("Calibration: ok =", reply.m_ok, "error =", reply.m_error));
  m_calibrationId = 0;
  m_calibrationFailed = !reply.m_ok;
  if (reply.m_ok)
    m_calibratedMs = m_clock.NowMs();
}

std::optional<Esp32Source::PowerOn> Esp32Source::GetPowerOn() const
{
  if (!m_poweredOn || !m_hasLast || !IsConnected())
    return {};
  // The box counts its time from its start.
  int64_t const uptimeMs = m_lastTimeMs + m_clock.NowMs() - *m_lastDataMs;
  return PowerOn{m_clock.UnixNowMs() - uptimeMs, uptimeMs};
}

bool Esp32Source::IsConnected() const
{
  return m_running && m_lastDataMs && m_clock.NowMs() - *m_lastDataMs <= kDataTimeoutMs;
}

SourceState Esp32Source::GetState() const
{
  if (!m_running)
    return SourceState::Disconnected;
  // The box sends no data while it takes a firmware and restarts with it.
  if (m_update.IsActive())
    return SourceState::Updating;
  if (!IsConnected())
    return IsSleeping() ? SourceState::BoxSleeping : SourceState::Connecting;
  if (m_flags & esp32::kFlagObdOk)
    return SourceState::Connected;
  if (m_obdState == "SLEEP")
    return SourceState::ObdSleeping;
  // OBD_ABSENT is set also when ELM327 is off in the box, only the status tells one from another.
  if (m_obdAdapterMissing)
    return SourceState::NoAdapter;
  if (!m_obdState)
    return SourceState::NoCarData;
  if (*m_obdState == "DISABLED")
    return SourceState::ObdDisabled;
  if (*m_obdState == "INIT" || *m_obdState == "SEARCHING")
    return SourceState::ObdConnecting;
  if (*m_obdState == "ERROR")
    return SourceState::ObdError;
  // NO_CAR and NO_DATA: the adapter answers, the car doesn't.
  return SourceState::NoCarData;
}

bool Esp32Source::IsSleeping() const
{
  // The box switches its radio off a minute after the engine is stopped, so a box that has gone after it has told
  // about the stopped engine is not lost.
  if (!m_lastDataMs)
    return false;
  return m_obdState == "SLEEP" || (m_carInfo && m_carInfo->m_engineRunning == false);
}

std::optional<CarInfo> Esp32Source::GetCarInfo() const
{
  if (!IsConnected() || !m_carInfo)
    return {};
  CarInfo car = *m_carInfo;
  car.m_voltageMismatch = m_voltageMismatch;
  return car;
}

bool Esp32Source::IsCalibrated() const
{
  int constexpr kRequired = esp32::kFlagImuOk | esp32::kFlagBiasOk | esp32::kFlagUpOk;
  return IsConnected() && (m_flags & kRequired) == kRequired && (m_flags & esp32::kFlagMountMoved) == 0;
}

CalibrationState Esp32Source::GetCalibrationState()
{
  if (m_calibrationId != 0 && m_clock.NowMs() - m_calibrationStartMs > kCalibrationTimeoutMs)
  {
    LOG(LWARNING, ("No reply to the calibration"));
    m_calibrationId = 0;
    m_calibrationFailed = true;
  }
  if (m_calibrationId != 0)
    return CalibrationState::Calibrating;
  if (m_calibrationFailed)
    return CalibrationState::FailedMoving;
  if (IsConnected() && (m_flags & esp32::kFlagMountMoved))
    return CalibrationState::MountMoved;
  return IsCalibrated() ? CalibrationState::Done : CalibrationState::None;
}

void Esp32Source::Calibrate()
{
  // The box zeroes the gyroscope and finds its vertical in the car at once.
  m_calibrationId = m_nextCommandId++;
  m_calibrationStartMs = m_clock.NowMs();
  m_calibrationProgress = 0;
  m_calibrationFailed = false;
  LOG(LINFO, ("Calibration, id =", m_calibrationId));
  Send("CAL_UP", m_calibrationId);
}

std::string Esp32Source::GetDeviceName() const
{
  return m_imuName.empty() ? "ESP32" : "ESP32 · " + m_imuName;
}
}  // namespace nogps

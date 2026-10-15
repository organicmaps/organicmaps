#include "testing/testing.hpp"

#include "map/map_tests/nogps_test_env.hpp"

#include "map/nogps/elm327_session.hpp"
#include "map/nogps/esp32_source.hpp"
#include "map/nogps/phone_motion.hpp"

#include "base/math.hpp"
#include "base/string_utils.hpp"

#include <algorithm>
#include <cstdint>
#include <string>
#include <utility>
#include <vector>

namespace nogps_sources_tests
{
using namespace nogps;
using namespace nogps_test;

class SpeedRecorder : public Elm327Session::Listener
{
public:
  void OnSpeed(int speedKmh, int64_t) override { m_speeds.push_back(speedKmh); }

  std::vector<int> m_speeds;
};

class MotionRecorder : public MotionSource::Listener
{
public:
  void OnSpeed(int speedKmh, int64_t) override { m_speeds.push_back(speedKmh); }
  void OnMotion(double yawDeltaDeg, double dtSec, int64_t) override
  {
    m_yawDeg += yawDeltaDeg;
    m_dtSec += dtSec;
    ++m_motions;
  }

  std::vector<int> m_speeds;
  double m_yawDeg = 0;
  double m_dtSec = 0;
  int m_motions = 0;
};

// Answers the initialization of the adapter.
void InitElm327(TestClock & clock, TestDelegate & delegate, Elm327Session & session)
{
  session.OnConnected();
  for (char const * command : {"ATZ", "ATE0", "ATL0", "ATS0", "ATH0", "ATSP0"})
  {
    TEST_EQUAL(delegate.m_elm327Writes.back(), std::string(command) + "\r", ());
    clock.Advance(50);
    session.OnBytes("OK\r\r>");
  }
}

UNIT_TEST(NoGps_Elm327Session_InitializesAndPollsSpeed)
{
  TestClock clock;
  TestDelegate delegate;
  SpeedRecorder listener;
  Elm327Session session(delegate, clock, clock, "AA:BB", listener);
  session.Start();
  TEST_EQUAL(delegate.m_elm327Connects, std::vector<std::string>{"AA:BB"}, ());
  TEST_EQUAL(session.GetState(), SourceState::Connecting, ());

  InitElm327(clock, delegate, session);
  TEST_EQUAL(session.GetState(), SourceState::NoCarData, ());
  TEST_EQUAL(delegate.m_elm327Writes.back(), "010D\r", ());

  // The response comes in pieces.
  session.OnBytes("SEARCHING...\r41 0D 3C\r\r");
  TEST(listener.m_speeds.empty(), ());
  session.OnBytes(">");
  TEST_EQUAL(listener.m_speeds, std::vector<int>{60}, ());
  TEST_EQUAL(session.GetState(), SourceState::Connected, ());

  // The next request waits a little not to flood the car bus.
  size_t const writes = delegate.m_elm327Writes.size();
  clock.Advance(Elm327Session::kRequestIntervalMs - 1);
  TEST_EQUAL(delegate.m_elm327Writes.size(), writes, ());
  clock.Advance(1);
  TEST_EQUAL(delegate.m_elm327Writes.size(), writes + 1, ());
  TEST_EQUAL(delegate.m_elm327Writes.back(), "010D\r", ());
}

UNIT_TEST(NoGps_Elm327Session_ReconnectsWithoutCarData)
{
  TestClock clock;
  TestDelegate delegate;
  SpeedRecorder listener;
  Elm327Session session(delegate, clock, clock, "AA:BB", listener);
  session.Start();
  InitElm327(clock, delegate, session);
  session.OnBytes("41 0D 10\r\r>");
  TEST_EQUAL(session.GetState(), SourceState::Connected, ());

  // The ignition is off: the car doesn't answer, the adapter is reset after several requests.
  for (int i = 0; i < Elm327Session::kMaxNoDataRequests; ++i)
  {
    clock.Advance(Elm327Session::kRequestIntervalMs);
    TEST_EQUAL(delegate.m_elm327Writes.back(), "010D\r", ());
    session.OnBytes("NO DATA\r\r>");
    TEST_EQUAL(delegate.m_elm327Closes, i + 1 == Elm327Session::kMaxNoDataRequests ? 1 : 0, ());
    if (i + 1 < Elm327Session::kMaxNoDataRequests)
      clock.Advance(Elm327Session::kReconnectDelayMs - Elm327Session::kRequestIntervalMs);
  }
  TEST_EQUAL(session.GetState(), SourceState::Disconnected, ());
  clock.Advance(Elm327Session::kReconnectDelayMs);
  TEST_EQUAL(delegate.m_elm327Connects.size(), 2, ());
  TEST_EQUAL(session.GetState(), SourceState::Connecting, ());
}

UNIT_TEST(NoGps_Elm327Session_NoAdapterOnTimeout)
{
  TestClock clock;
  TestDelegate delegate;
  SpeedRecorder listener;
  Elm327Session session(delegate, clock, clock, "AA:BB", listener);
  session.Start();
  session.OnConnected();
  // The adapter doesn't answer the reset.
  clock.Advance(Elm327Session::kResponseTimeoutMs);
  TEST_EQUAL(delegate.m_elm327Closes, 1, ());
  TEST_EQUAL(session.GetState(), SourceState::NoAdapter, ());

  // While the adapter doesn't answer, the reconnections don't blink with "connecting".
  clock.Advance(Elm327Session::kReconnectDelayMs);
  TEST_EQUAL(delegate.m_elm327Connects.size(), 2, ());
  TEST_EQUAL(session.GetState(), SourceState::NoAdapter, ());
  session.OnClosed("Connection refused");
  TEST_EQUAL(session.GetState(), SourceState::NoAdapter, ());
  clock.Advance(Elm327Session::kReconnectDelayMs);
  TEST_EQUAL(delegate.m_elm327Connects.size(), 3, ());

  session.Stop();
  TEST_EQUAL(session.GetState(), SourceState::Disconnected, ());
  clock.Advance(10 * Elm327Session::kReconnectDelayMs);
  TEST_EQUAL(delegate.m_elm327Connects.size(), 3, ());
}

int constexpr kCalibratedFlags = esp32::kFlagImuOk | esp32::kFlagBiasOk | esp32::kFlagUpOk | esp32::kFlagObdOk;

UNIT_TEST(NoGps_Esp32Source_SendsHelloAndReadsData)
{
  TestClock clock;
  TestDelegate delegate;
  MotionRecorder listener;
  Esp32Source source(delegate, clock, clock, listener);
  source.Start();
  TEST(delegate.m_esp32BleOpen, ());
  // The first HELLO had nobody to go to.
  source.OnBleState(BleState::Connected);
  TEST_EQUAL(delegate.m_esp32BleSent, std::vector<std::string>{esp32::Command(2, "HELLO")}, ());
  clock.Advance(Esp32Source::kHelloIntervalMs);
  TEST_EQUAL(delegate.m_esp32BleSent.back(), esp32::Command(3, "HELLO"), ());
  TEST_EQUAL(source.GetState(), SourceState::Connecting, ());

  source.OnBleBytes(BoxDataLine(1, 5000, 0, 36, kCalibratedFlags));
  clock.Advance(20);
  source.OnBleBytes(BoxDataLine(2, 5020, 500, 36, kCalibratedFlags));
  TEST_EQUAL(listener.m_speeds, std::vector<int>({36, 36}), ());
  TEST_ALMOST_EQUAL_ABS(listener.m_yawDeg, 0.5, 1e-9, ());
  TEST_ALMOST_EQUAL_ABS(listener.m_dtSec, 0.02, 1e-9, ());
  TEST_EQUAL(source.GetState(), SourceState::Connected, ());
  TEST(source.IsCalibrated(), ());
  TEST_EQUAL(source.GetCalibrationState(), CalibrationState::Done, ());

  // A lost line loses nothing: the box sends the totals.
  clock.Advance(40);
  source.OnBleBytes(BoxDataLine(4, 5060, 1500, 36, kCalibratedFlags));
  TEST_ALMOST_EQUAL_ABS(listener.m_yawDeg, 1.5, 1e-9, ());
  TEST_ALMOST_EQUAL_ABS(listener.m_dtSec, 0.06, 1e-9, ());

  // A broken line is ignored.
  source.OnBleBytes("$NGD,1,5,5080,1500,0,0,36,0,17*0000\r\n");
  TEST_EQUAL(listener.m_motions, 2, ());

  clock.Advance(Esp32Source::kDataTimeoutMs + 1);
  TEST_EQUAL(source.GetState(), SourceState::Connecting, ());
  TEST(!source.IsCalibrated(), ());
}

UNIT_TEST(NoGps_Esp32Source_ReadsLinesOfBluetoothStream)
{
  TestClock clock;
  TestDelegate delegate;
  MotionRecorder listener;
  Esp32Source source(delegate, clock, clock, listener);
  source.Start();
  TEST(delegate.m_esp32BleOpen, ());
  source.OnBleState(BleState::Connected);

  // Two lines in a piece.
  source.OnBleBytes(BoxDataLine(1, 5000, 0, 36, kCalibratedFlags) + BoxDataLine(2, 5020, 500, 36, kCalibratedFlags));
  TEST_EQUAL(listener.m_speeds, std::vector<int>({36, 36}), ());
  TEST_ALMOST_EQUAL_ABS(listener.m_yawDeg, 0.5, 1e-9, ());
  TEST_EQUAL(source.GetState(), SourceState::Connected, ());

  // A line in three pieces.
  std::string const line = BoxDataLine(3, 5040, 1000, 37, kCalibratedFlags);
  source.OnBleBytes(line.substr(0, 7));
  source.OnBleBytes(line.substr(7, 20));
  TEST_EQUAL(listener.m_speeds.size(), 2, ());
  source.OnBleBytes(line.substr(27));
  TEST_EQUAL(listener.m_speeds.back(), 37, ());
  TEST_ALMOST_EQUAL_ABS(listener.m_yawDeg, 1.0, 1e-9, ());

  // The end of a line whose start was lost, a line cut in the middle and a whole one after them.
  source.OnBleBytes("234,0,0,36,0,17*ABCD\r\n" + BoxDataLine(4, 5060, 1500, 38, kCalibratedFlags).substr(0, 12) +
                    BoxDataLine(5, 5080, 2000, 39, kCalibratedFlags));
  TEST_EQUAL(listener.m_speeds.back(), 39, ());
  TEST_EQUAL(listener.m_speeds.size(), 4, ());
  TEST_ALMOST_EQUAL_ABS(listener.m_yawDeg, 2.0, 1e-9, ());

  source.Stop();
  TEST(!delegate.m_esp32BleOpen, ());
}

UNIT_TEST(NoGps_Esp32Source_CallsBoxWhileConnectedOnly)
{
  // Nothing is sent until the phone is connected, then HELLO goes at once and every second.
  TestClock clock;
  TestDelegate delegate;
  MotionRecorder listener;
  Esp32Source source(delegate, clock, clock, listener);
  source.Start();
  source.OnBleState(BleState::Searching);
  clock.Advance(Esp32Source::kHelloIntervalMs);
  TEST(delegate.m_esp32BleSent.empty(), ());
  TEST_EQUAL(source.GetBleState(), BleState::Searching, ());
  source.OnBleState(BleState::Connected);
  TEST_EQUAL(delegate.m_esp32BleSent.size(), 1, ());
  clock.Advance(Esp32Source::kHelloIntervalMs);
  TEST_EQUAL(delegate.m_esp32BleSent.size(), 2, ());
  source.OnBleState(BleState::Searching);
  clock.Advance(Esp32Source::kHelloIntervalMs);
  TEST_EQUAL(delegate.m_esp32BleSent.size(), 2, ());
}

UNIT_TEST(NoGps_Esp32Source_CountsDataLinesOfLastSecond)
{
  TestClock clock;
  TestDelegate delegate;
  MotionRecorder listener;
  Esp32Source source(delegate, clock, clock, listener);
  source.Start();
  source.OnBleState(BleState::Connected);
  TEST_EQUAL(source.GetDataRate().m_lines, 0, ());
  TEST_EQUAL(source.GetDataRate().m_maxGapMs, Esp32Source::kDataTimeoutMs, ());

  // 50 lines a second, then two lines are lost.
  int64_t boxTimeMs = 5000;
  for (int i = 0; i < 100; ++i)
  {
    clock.Advance(20);
    source.OnBleBytes(BoxDataLine(i, boxTimeMs += 20, 0, 36, kCalibratedFlags));
  }
  TEST_EQUAL(source.GetDataRate().m_lines, 50, ());
  TEST_EQUAL(source.GetDataRate().m_maxGapMs, 20, ());
  clock.Advance(60);
  source.OnBleBytes(BoxDataLine(100, boxTimeMs += 60, 0, 36, kCalibratedFlags));
  TEST_EQUAL(source.GetDataRate().m_lines, 48, ());
  TEST_EQUAL(source.GetDataRate().m_maxGapMs, 60, ());

  // The box is silent.
  clock.Advance(500);
  TEST_EQUAL(source.GetDataRate().m_maxGapMs, 500, ());
  clock.Advance(Esp32Source::kDataTimeoutMs);
  TEST_EQUAL(source.GetDataRate().m_lines, 0, ());
}

UNIT_TEST(NoGps_Esp32Source_CalibratesAndTimesOut)
{
  TestClock clock;
  TestDelegate delegate;
  MotionRecorder listener;
  Esp32Source source(delegate, clock, clock, listener);
  source.Start();
  source.OnBleState(BleState::Connected);
  source.OnBleBytes(BoxDataLine(1, 5000, 0, 0, kCalibratedFlags));

  source.Calibrate();
  TEST_EQUAL(delegate.m_esp32BleSent.back(), esp32::Command(1, "CAL_UP"), ());
  TEST_EQUAL(source.GetCalibrationState(), CalibrationState::Calibrating, ());
  // A reply to another command doesn't count.
  source.OnBleBytes(BoxLine("NGA,7,OK"));
  TEST_EQUAL(source.GetCalibrationState(), CalibrationState::Calibrating, ());
  source.OnBleBytes(BoxLine("NGA,1,PROGRESS,40"));
  TEST_EQUAL(source.GetCalibrationProgressPercent(), 40, ());
  source.OnBleBytes(BoxLine("NGA,1,OK"));
  TEST_EQUAL(source.GetCalibrationState(), CalibrationState::Done, ());

  source.Calibrate();
  TEST_EQUAL(delegate.m_esp32BleSent.back(), esp32::Command(2, "CAL_UP"), ());
  source.OnBleBytes(BoxLine("NGA,2,ERR,MOVING"));
  TEST_EQUAL(source.GetCalibrationState(), CalibrationState::FailedMoving, ());

  // The box doesn't answer.
  source.Calibrate();
  clock.Advance(Esp32Source::kCalibrationTimeoutMs + 1);
  TEST_EQUAL(source.GetCalibrationState(), CalibrationState::FailedMoving, ());
}

UNIT_TEST(NoGps_Esp32Source_RequestsLostEvents)
{
  TestClock clock;
  TestDelegate delegate;
  MotionRecorder listener;
  Esp32Source source(delegate, clock, clock, listener);
  source.Start();
  source.OnBleState(BleState::Connected);
  auto const sent = [&delegate] { return delegate.m_esp32BleSent.size(); };

  // The events before the connection are asked from the journal of the box.
  size_t count = sent();
  source.OnBleBytes(BoxLine("NGE,1,5,1000,W,OBD_LOST,no answer"));
  TEST_EQUAL(sent(), count + 1, ());
  TEST_EQUAL(delegate.m_esp32BleSent.back(), esp32::Command(1, "EVENTS,0"), ());
  // The next event in a row asks nothing.
  count = sent();
  source.OnBleBytes(BoxLine("NGE,1,6,1100,I,OBD_OK,"));
  TEST_EQUAL(sent(), count, ());
  // A gap is asked not more often than once in a while.
  source.OnBleBytes(BoxLine("NGE,1,8,1200,I,CAL_AUTO,-386"));
  TEST_EQUAL(sent(), count, ());
  clock.Advance(Esp32Source::kEventsRequestIntervalMs);
  source.OnBleBytes(BoxLine("NGE,1,10,1300,I,CAL_AUTO,-386"));
  TEST_EQUAL(delegate.m_esp32BleSent.back(), esp32::Command(2, "EVENTS,9"), ());
  // The status tells the last event of the box.
  clock.Advance(Esp32Source::kEventsRequestIntervalMs);
  source.OnBleBytes(BoxLine("NGS,1,0.2.0,ICM20602,500,OK,,12,331,0,OK,0x12,NONE,38400,-386,12"));
  TEST_EQUAL(delegate.m_esp32BleSent.back(), esp32::Command(3, "EVENTS,11"), ());
}

UNIT_TEST(NoGps_Esp32Source_TellsWhenBoxWasPoweredOn)
{
  std::string const poweredOn = BoxLine("NGE,1,1,40,I,BOOT,fw 0.3.3 reset POWERON");
  {
    TestClock clock;
    TestDelegate delegate;
    MotionRecorder listener;
    Esp32Source source(delegate, clock, clock, listener);
    source.Start();
    source.OnBleState(BleState::Connected);

    // The journal of the box tells how it has started.
    source.OnBleBytes(BoxDataLine(1, 5000, 0, 0, kCalibratedFlags));
    TEST(!source.GetPowerOn(), ());
    source.OnBleBytes(poweredOn);
    TEST(source.GetPowerOn(), ());
    TEST_EQUAL(source.GetPowerOn()->m_uptimeMs, 5000, ());
    TEST_EQUAL(source.GetPowerOn()->m_unixMs, clock.UnixNowMs() - 5000, ());

    // The same power-on later.
    clock.Advance(600);
    source.OnBleBytes(BoxDataLine(2, 5600, 900, 0, kCalibratedFlags));
    clock.Advance(300);
    TEST_EQUAL(source.GetPowerOn()->m_uptimeMs, 5900, ());
    TEST_EQUAL(source.GetPowerOn()->m_unixMs, clock.UnixNowMs() - 5900, ());
    TEST_EQUAL(listener.m_motions, 1, ());

    // The box has restarted by itself: it counts its time, rotation and events from the start. The car has not
    // turned back, and the box stays where it was.
    clock.Advance(400);
    source.OnBleBytes(BoxDataLine(1, 1200, 0, 0, kCalibratedFlags));
    TEST(!source.GetPowerOn(), ());
    TEST_EQUAL(listener.m_motions, 1, ());
    TEST_ALMOST_EQUAL_ABS(listener.m_yawDeg, 0.9, 1e-9, ());
    source.OnBleBytes(BoxLine("NGE,1,1,40,I,BOOT,fw 0.3.3 reset SW"));
    TEST(!source.GetPowerOn(), ());

    // It was taken out and plugged in again.
    clock.Advance(500);
    source.OnBleBytes(BoxDataLine(1, 800, 0, 0, kCalibratedFlags));
    TEST(!source.GetPowerOn(), ());
    source.OnBleBytes(poweredOn);
    TEST(source.GetPowerOn(), ());
    TEST_EQUAL(source.GetPowerOn()->m_uptimeMs, 800, ());

    // The box is not heard.
    clock.Advance(Esp32Source::kDataTimeoutMs + 1);
    TEST(!source.GetPowerOn(), ());
  }
  {
    // The time of the box is 32 bits and wraps around in 50 days: this is not a restart.
    TestClock clock;
    TestDelegate delegate;
    MotionRecorder listener;
    Esp32Source source(delegate, clock, clock, listener);
    source.Start();
    source.OnBleState(BleState::Connected);
    source.OnBleBytes(BoxDataLine(1, Esp32Source::kBoxTimeWrapMs - 10, 0, 0, kCalibratedFlags));
    source.OnBleBytes(poweredOn);
    clock.Advance(20);
    source.OnBleBytes(BoxDataLine(2, 10, 500, 0, kCalibratedFlags));
    TEST(source.GetPowerOn(), ());
    TEST_EQUAL(listener.m_motions, 1, ());
    TEST_ALMOST_EQUAL_ABS(listener.m_dtSec, 0.02, 1e-9, ());
  }
  {
    // Another box may be found after a connection is lost: its journal is read from the start.
    TestClock clock;
    TestDelegate delegate;
    MotionRecorder listener;
    Esp32Source source(delegate, clock, clock, listener);
    source.Start();
    source.OnBleState(BleState::Connected);
    source.OnBleBytes(BoxDataLine(1, 5000, 0, 0, kCalibratedFlags));
    source.OnBleBytes(poweredOn);
    TEST(source.GetPowerOn(), ());

    source.OnBleState(BleState::Searching);
    clock.Advance(3000);
    source.OnBleState(BleState::Connected);
    source.OnBleBytes(BoxDataLine(151, 8000, 0, 0, kCalibratedFlags));
    TEST(!source.GetPowerOn(), ());
    source.OnBleBytes(poweredOn);
    TEST(source.GetPowerOn(), ());
    TEST_EQUAL(source.GetPowerOn()->m_unixMs, clock.UnixNowMs() - 8000, ());
  }
}

UNIT_TEST(NoGps_Esp32Source_TellsSleepingBox)
{
  TestClock clock;
  TestDelegate delegate;
  MotionRecorder listener;
  Esp32Source source(delegate, clock, clock, listener);
  source.Start();
  source.OnBleState(BleState::Connected);
  source.OnBleBytes(BoxDataLine(1, 5000, 0, 0, esp32::kFlagImuOk));
  TEST_EQUAL(source.GetState(), SourceState::NoCarData, ());
  source.OnBleBytes(BoxLine("NGS,1,0.2.0,ICM20602,500,NO_ADAPTER,,,331,0,OK,0x12,NO_ADAPTER,0,-386,4,,,"));
  TEST_EQUAL(source.GetState(), SourceState::NoAdapter, ());
  // The adapter is searched again: it is still missing.
  source.OnBleBytes(BoxLine("NGS,1,0.2.0,ICM20602,500,INIT,,,331,0,OK,0x12,NONE,0,-386,4,,,"));
  TEST_EQUAL(source.GetState(), SourceState::NoAdapter, ());
  source.OnBleBytes(BoxLine("NGS,1,0.2.0,ICM20602,500,SEARCHING,,,331,0,OK,0x12,NONE,0,-386,4,,,"));
  TEST_EQUAL(source.GetState(), SourceState::ObdConnecting, ());

  // The engine is stopped: the box switches its radio off later, it is not lost.
  source.OnBleBytes(BoxLine("NGS,1,0.2.0,ICM20602,500,OK,,12,331,0,OK,0x12,NONE,38400,-386,14,OFF,,12480"));
  TEST(source.GetCarInfo(), ());
  TEST_EQUAL(source.GetCarInfo()->m_engineRunning, false, ());
  clock.Advance(Esp32Source::kDataTimeoutMs + 1);
  TEST_EQUAL(source.GetState(), SourceState::BoxSleeping, ());
  TEST(!source.GetCarInfo(), ());

  source.Stop();
  TEST(!delegate.m_esp32BleOpen, ());
  TEST_EQUAL(source.GetState(), SourceState::Disconnected, ());
}

// The box connected over Bluetooth that has told about itself and takes firmwares.
class UpdatedBox
{
public:
  explicit UpdatedBox(size_t imageSize = 1000)
  {
    for (size_t i = 0; i < imageSize; ++i)
      m_image += static_cast<char>(i * 7 % 251);
    m_source.Start();
    Connect("0.3.2");
  }

  void Connect(std::string const & firmware)
  {
    m_source.OnBleState(BleState::Connected);
    Line("NGI,1," + firmware + ",2026-10-10T07:33,esp32s3,s3zero,C47D,4096,2031616,ota_0,VALID,WIFI BLE OTA");
  }

  void Line(std::string const & body) { m_source.OnBleBytes(BoxLine(body)); }

  void StartUpdate()
  {
    m_source.StartUpdate({"0.3.3", "esp32s3", "s3zero", [this] { return m_image; }});
  }

  /// The box accepts the last command.
  void Accept(std::string const & values = {}) { Line("NGA," + std::to_string(LastCommand().first) + ",OK" + values); }
  void Refuse(std::string const & error) { Line("NGA," + std::to_string(LastCommand().first) + ",ERR," + error); }

  /// \returns the id and the text of the last command but HELLO sent to the box.
  std::pair<int, std::string> LastCommand() const
  {
    for (auto it = m_delegate.m_esp32BleSent.rbegin(); it != m_delegate.m_esp32BleSent.rend(); ++it)
    {
      auto const fields = esp32::Parse(*it);
      TEST(fields && fields->size() >= 3 && (*fields)[0] == "NGC", (*it));
      if ((*fields)[2] == "HELLO")
        continue;
      int id = 0;
      TEST(strings::to_int((*fields)[1], id), ());
      std::string command = (*fields)[2];
      for (size_t i = 3; i < fields->size(); ++i)
        command += "," + (*fields)[i];
      return {id, command};
    }
    return {};
  }

  /// \returns where the pieces sent since the previous call start, and writes them to the firmware of the box.
  std::vector<size_t> TakePieces()
  {
    std::vector<size_t> offsets;
    for (auto const & piece : m_delegate.m_firmwarePieces)
    {
      TEST_GREATER(piece.size(), 4, ());
      size_t offset = 0;
      for (size_t i = 0; i < 4; ++i)
        offset |= static_cast<size_t>(static_cast<unsigned char>(piece[i])) << (8 * i);
      offsets.push_back(offset);
      if (m_written.size() < offset + piece.size() - 4)
        m_written.resize(offset + piece.size() - 4);
      m_written.replace(offset, piece.size() - 4, piece, 4);
    }
    m_delegate.m_firmwarePieces.clear();
    return offsets;
  }

  Esp32Update const & Update() const { return m_source.GetUpdate(); }

  /// The whole firmware is sent and accepted, the box restarts.
  void SendAllAndRestart()
  {
    StartUpdate();
    Accept(",240,4096");
    Line("NGO,1," + std::to_string(m_image.size()) + ",RECV");
    TEST_EQUAL(LastCommand().second, "OTA_END", ());
    Accept();
    m_source.OnBleState(BleState::Searching);
  }

  TestClock m_clock;
  TestDelegate m_delegate;
  MotionRecorder m_listener;
  Esp32Source m_source{m_delegate, m_clock, m_clock, m_listener};
  std::string m_image;
  // The firmware as the box has got it.
  std::string m_written;
};

UNIT_TEST(NoGps_Esp32Update_SendsFirmwareInPieces)
{
  UpdatedBox box;
  TEST(box.m_source.CanUpdate(), ());
  TEST_EQUAL(box.m_source.GetInfo()->m_firmware, "0.3.2", ());

  box.StartUpdate();
  auto const begin = box.LastCommand().second;
  TEST(begin.starts_with("OTA_BEGIN,1000,") && begin.ends_with(",0.3.3"), (begin));
  TEST_EQUAL(box.Update().GetState(), Esp32Update::State::Starting, ());
  TEST_EQUAL(box.m_source.GetState(), SourceState::Updating, ());
  TEST(!box.m_source.CanUpdate(), ());
  TEST(box.TakePieces().empty(), ());

  // The box takes pieces of 240 bytes not farther than 480 bytes from what it has written.
  box.Accept(",240,480");
  TEST_EQUAL(box.Update().GetState(), Esp32Update::State::Sending, ());
  TEST_EQUAL(box.TakePieces(), std::vector<size_t>({0, 240}), ());
  box.Line("NGO,1,240,RECV");
  TEST_EQUAL(box.TakePieces(), std::vector<size_t>({480}), ());
  TEST_EQUAL(box.Update().GetProgressPercent(), 24, ());
  // The same again: nothing new fits.
  box.Line("NGO,1,240,RECV");
  TEST(box.TakePieces().empty(), ());

  // The piece at 240 was lost: the box has dropped the one after it and tells where it stays.
  box.Line("NGO,1,240,RESEND");
  TEST_EQUAL(box.TakePieces(), std::vector<size_t>({240, 480}), ());
  box.Line("NGO,1,720,RECV");
  TEST_EQUAL(box.TakePieces(), std::vector<size_t>({720, 960}), ());
  TEST_EQUAL(box.m_written, box.m_image, ());

  box.Line("NGO,1,1000,RECV");
  TEST_EQUAL(box.LastCommand().second, "OTA_END", ());
  TEST_EQUAL(box.Update().GetState(), Esp32Update::State::Verifying, ());
  box.Line("NGO,1,1000,VERIFY");
  box.Line("NGO,1,1000,DONE");
  TEST_EQUAL(box.Update().GetState(), Esp32Update::State::Restarting, ());
  box.Accept();
  TEST_EQUAL(box.Update().GetProgressPercent(), 100, ());

  // The box restarts and comes back with the new firmware.
  box.m_source.OnBleState(BleState::Searching);
  TEST(!box.m_source.GetInfo(), ());
  TEST_EQUAL(box.m_source.GetState(), SourceState::Updating, ());
  box.Connect("0.3.3");
  TEST_EQUAL(box.Update().GetState(), Esp32Update::State::Done, ());
  TEST_EQUAL(box.m_source.GetState(), SourceState::Connecting, ());
  TEST(box.m_source.CanUpdate(), ());
  TEST(box.TakePieces().empty(), ());
}

UNIT_TEST(NoGps_Esp32Update_TellsSizeAndHashOfFirmware)
{
  UpdatedBox box;
  box.m_image = "abc";
  box.StartUpdate();
  TEST_EQUAL(box.LastCommand().second,
             "OTA_BEGIN,3,ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad,0.3.3", ());
  box.Accept(",240,4096");
  TEST_EQUAL(box.m_delegate.m_firmwarePieces, std::vector<std::string>({std::string("\0\0\0\0abc", 7)}), ());

  // A firmware that can't be read is not sent.
  UpdatedBox noFile;
  noFile.m_image.clear();
  noFile.StartUpdate();
  TEST_EQUAL(noFile.Update().GetState(), Esp32Update::State::Failed, ());
  TEST_EQUAL(noFile.Update().GetError(), "NO_FILE", ());
  TEST(noFile.m_delegate.m_firmwarePieces.empty(), ());
}

UNIT_TEST(NoGps_Esp32Update_FailsAndStartsAgain)
{
  UpdatedBox box;
  auto const testFailed = [&box](std::string const & error)
  {
    TEST_EQUAL(box.Update().GetState(), Esp32Update::State::Failed, ());
    TEST_EQUAL(box.Update().GetError(), error, ());
    TEST(box.m_source.GetState() != SourceState::Updating, ());
  };

  // The car drives: the box doesn't take a firmware.
  box.StartUpdate();
  box.Refuse("MOVING");
  testFailed("MOVING");
  TEST(box.m_source.CanUpdate(), ());

  // The box finds the firmware made for another chip by its first piece.
  box.StartUpdate();
  box.Accept(",240,4096");
  TEST_EQUAL(box.TakePieces().size(), 5, ());
  box.Line("NGO,1,0,ERR_CHIP");
  testFailed("CHIP");

  // The firmware has come damaged.
  box.StartUpdate();
  box.Accept(",240,4096");
  box.Line("NGO,1,1000,RECV");
  box.Refuse("HASH");
  testFailed("HASH");

  // The connection is lost on the way: the update is not continued.
  box.StartUpdate();
  box.Accept(",240,4096");
  box.m_source.OnBleState(BleState::Searching);
  testFailed("LINK");
  TEST(!box.m_source.CanUpdate(), ());
  box.Connect("0.3.2");

  // The user stops it.
  box.StartUpdate();
  box.Accept(",240,4096");
  box.m_source.CancelUpdate();
  TEST_EQUAL(box.LastCommand().second, "OTA_ABORT", ());
  testFailed("CANCELED");

  // The new firmware didn't start: the box has come back with the previous one.
  box.SendAllAndRestart();
  TEST_EQUAL(box.Update().GetState(), Esp32Update::State::Restarting, ());
  box.Connect("0.3.2");
  testFailed("ROLLBACK");

  // The reply to the end is lost with the connection of the restarting box.
  box.StartUpdate();
  box.Accept(",240,4096");
  box.Line("NGO,1,1000,RECV");
  box.m_source.OnBleState(BleState::Searching);
  TEST_EQUAL(box.Update().GetState(), Esp32Update::State::Restarting, ());
  box.Connect("0.3.3");
  TEST_EQUAL(box.Update().GetState(), Esp32Update::State::Done, ());
}

UNIT_TEST(NoGps_Esp32Update_SendsAgainAndGivesUp)
{
  {
    // The pieces are lost without a word of the box: they are sent again, then the update is given up.
    UpdatedBox box;
    box.StartUpdate();
    box.Accept(",240,480");
    TEST_EQUAL(box.TakePieces(), std::vector<size_t>({0, 240}), ());
    box.m_clock.Advance(Esp32Update::kResendMs + Esp32Update::kCheckIntervalMs);
    TEST_EQUAL(box.TakePieces(), std::vector<size_t>({0, 240}), ());
    box.m_clock.Advance(Esp32Update::kStallMs);
    TEST_EQUAL(box.Update().GetState(), Esp32Update::State::Failed, ());
    TEST_EQUAL(box.Update().GetError(), "TIMEOUT", ());
    TEST_EQUAL(box.LastCommand().second, "OTA_ABORT", ());
  }
  {
    // The box doesn't answer.
    UpdatedBox box;
    box.StartUpdate();
    box.m_clock.Advance(Esp32Update::kReplyTimeoutMs + Esp32Update::kCheckIntervalMs);
    TEST_EQUAL(box.Update().GetError(), "NO_REPLY", ());
  }
  {
    // The box doesn't come back after it has taken the firmware.
    UpdatedBox box;
    box.SendAllAndRestart();
    box.m_clock.Advance(Esp32Update::kRestartTimeoutMs + Esp32Update::kCheckIntervalMs);
    TEST_EQUAL(box.Update().GetError(), "NO_RESTART", ());
  }
}

UNIT_TEST(NoGps_Esp32Source_AsksBoxAboutItself)
{
  TestClock clock;
  TestDelegate delegate;
  MotionRecorder listener;
  Esp32Source source(delegate, clock, clock, listener);
  source.Start();
  source.OnBleState(BleState::Connected);
  auto const infoRequests = [&delegate]
  {
    return std::count_if(delegate.m_esp32BleSent.begin(), delegate.m_esp32BleSent.end(),
                         [](std::string const & line) { return line.find(",INFO*") != std::string::npos; });
  };
  // The box has been talking to the application before it was restarted: it doesn't tell about itself again.
  int64_t seq = 0;
  auto const hear = [&](int64_t ms)
  {
    for (int64_t t = 0; t < ms; t += 20)
    {
      clock.Advance(20);
      ++seq;
      source.OnBleBytes(BoxDataLine(seq, 5000 + seq * 20, 0, 0, kCalibratedFlags));
    }
  };
  hear(Esp32Source::kHelloIntervalMs * Esp32Source::kInfoRequestHellos);
  TEST_EQUAL(infoRequests(), 1, ());
  TEST(!source.CanUpdate(), ());
  source.OnBleBytes(BoxLine("NGI,1,0.3.3,2026-10-10T07:33,esp32s3,s3zero,C47D,4096,2031616,ota_0,VALID,WIFI BLE OTA"));
  TEST(source.CanUpdate(), ());
  hear(Esp32Source::kHelloIntervalMs * Esp32Source::kInfoRequestHellos * 2);
  TEST_EQUAL(infoRequests(), 1, ());

  // An old firmware doesn't know the question: it is not asked forever.
  source.OnBleState(BleState::Searching);
  source.OnBleState(BleState::Connected);
  hear(Esp32Source::kHelloIntervalMs * Esp32Source::kInfoRequestHellos * (Esp32Source::kMaxInfoRequests + 3));
  TEST_EQUAL(infoRequests(), 1 + Esp32Source::kMaxInfoRequests, ());
}

UNIT_TEST(NoGps_PhoneMotion_CalibratesAndTurns)
{
  TestClock clock;
  TestDelegate delegate;
  MotionRecorder listener;
  PhoneMotion phone(delegate, clock, clock, {} /* elm327Address */, listener);
  phone.Start();
  TEST_EQUAL(phone.GetState(), SourceState::Disconnected, ());
  // The phone lies flat screen up.
  phone.OnAccel({0, 0, 9.81});
  Vec3 const bias = {0.001, -0.002, 0.003};

  phone.Calibrate();
  for (int i = 0; i <= GyroCalibrator::kRequiredSamples; ++i)
  {
    clock.Advance(20);
    phone.OnGyro(clock.NowMs() * 1'000'000, bias);
  }
  TEST_EQUAL(phone.GetCalibrationState(), CalibrationState::Done, ());
  TEST(phone.IsCalibrated(), ());

  // Turning right at 10 deg/s is a negative rotation around the screen normal.
  double const yawBefore = listener.m_yawDeg;
  for (int i = 0; i < 50; ++i)
  {
    clock.Advance(20);
    phone.OnGyro(clock.NowMs() * 1'000'000, {bias[0], bias[1], bias[2] - math::DegToRad(10.0)});
  }
  TEST_ALMOST_EQUAL_ABS(listener.m_yawDeg - yawBefore, 10.0, 1e-6, ());

  // A break of the sensor is not a rotation.
  clock.Advance(1000);
  int const motions = listener.m_motions;
  phone.OnGyro(clock.NowMs() * 1'000'000, bias);
  TEST_EQUAL(listener.m_motions, motions, ());
}

UNIT_TEST(NoGps_PhoneMotion_CalibrationFailsWhileDriving)
{
  TestClock clock;
  TestDelegate delegate;
  MotionRecorder listener;
  PhoneMotion phone(delegate, clock, clock, std::string("AA:BB"), listener);
  phone.Start();
  TEST_EQUAL(delegate.m_elm327Connects, std::vector<std::string>{"AA:BB"}, ());
  phone.OnAccel({0, 9.81, 0});

  // The car drives straight: the phone is as still as in a standing car, but the speed tells it moves.
  auto * elm327 = phone.GetElm327();
  TEST(elm327, ());
  InitElm327(clock, delegate, *elm327);
  elm327->OnBytes("41 0D 28\r\r>");
  phone.Calibrate();
  for (int i = 0; i <= GyroCalibrator::kRequiredSamples; ++i)
  {
    clock.Advance(20);
    phone.OnGyro(clock.NowMs() * 1'000'000, {0, 0, 0});
  }
  TEST_EQUAL(phone.GetCalibrationState(), CalibrationState::FailedMoving, ());
  TEST(!phone.IsCalibrated(), ());
}
}  // namespace nogps_sources_tests

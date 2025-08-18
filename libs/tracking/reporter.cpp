#include "tracking/reporter.hpp"

#include "platform/location.hpp"
#include "platform/platform.hpp"
#include "platform/socket.hpp"

#include "base/logging.hpp"
#include "base/timer.hpp"

#include "std/target_os.hpp"

#include <cmath>

using namespace std;
using namespace std::chrono;

namespace
{
double constexpr kRequiredHorizontalAccuracy = 10.0;
double constexpr kMinDelaySeconds = 1.0;
double constexpr kReconnectDelaySeconds = 40.0;
double constexpr kNotChargingEventPeriod = 5 * 60.0;

static_assert(kMinDelaySeconds != 0, "");
}  // namespace

namespace tracking
{
char const Reporter::kEnableTrackingKey[] = "StatisticsEnabled";

// static
milliseconds const Reporter::kPushDelayMs = milliseconds(20000);

// Set m_points size to be enough to keep all points even if one reconnect attempt failed.
Reporter::Reporter(unique_ptr<platform::Socket> socket, string const & host, uint16_t port, milliseconds pushDelay)
  : m_allowSendingPoints(true)
  , m_realtimeSender(std::move(socket), host, port, false)
  , m_pushDelay(pushDelay)
  , m_points(ceil(duration_cast<seconds>(pushDelay).count() + kReconnectDelaySeconds) / kMinDelaySeconds)
  , m_thread([this] { Run(); })
{}

Reporter::~Reporter()
{
  {
    lock_guard<mutex> lg(m_mutex);
    m_isFinished = true;
  }
  m_cv.notify_one();
  m_thread.join();
}

void Reporter::AddLocation(location::GpsInfo const & info, traffic::SpeedGroup traffic)
{
  lock_guard<mutex> lg(m_mutex);

  if (info.m_horizontalAccuracy > kRequiredHorizontalAccuracy)
    return;

  if (info.m_timestamp < m_lastGpsTime + kMinDelaySeconds)
    return;

  if (Platform::GetChargingStatus() != Platform::ChargingStatus::Plugged)
  {
    double const currentTime = base::Timer::LocalTime();
    if (currentTime < m_lastNotChargingEvent + kNotChargingEventPeriod)
      return;

    m_lastNotChargingEvent = currentTime;
    return;
  }

  m_lastGpsTime = info.m_timestamp;
  m_input.push_back(DataPoint(info.m_timestamp, ms::LatLon(info.m_latitude, info.m_longitude),
                              static_cast<std::underlying_type<traffic::SpeedGroup>::type>(traffic)));
}

void Reporter::Run()
{
  LOG(LINFO, ("Tracking Reporter started"));

  unique_lock<mutex> lock(m_mutex);

  while (!m_isFinished)
  {
    auto const startTime = steady_clock::now();

    // Fetch input.
    m_points.insert(m_points.end(), m_input.begin(), m_input.end());
    m_input.clear();

    lock.unlock();
    if (m_points.empty() && m_idleFn)
      m_idleFn();
    else if (SendPoints())
      m_points.clear();
    lock.lock();

    auto const passedMs = duration_cast<milliseconds>(steady_clock::now() - startTime);
    if (passedMs < m_pushDelay)
      m_cv.wait_for(lock, m_pushDelay - passedMs, [this] { return m_isFinished; });
  }

  LOG(LINFO, ("Tracking Reporter finished"));
}

bool Reporter::SendPoints()
{
  if (!m_allowSendingPoints)
  {
    if (m_wasConnected)
    {
      m_realtimeSender.Shutdown();
      m_wasConnected = false;
    }
    return true;
  }

  if (m_points.empty())
    return true;

  if (m_wasConnected)
    m_wasConnected = m_realtimeSender.Send(m_points);

  if (m_wasConnected)
    return true;

  double const currentTime = base::Timer::LocalTime();
  if (currentTime < m_lastConnectionAttempt + kReconnectDelaySeconds)
    return false;

  m_lastConnectionAttempt = currentTime;

  m_wasConnected = m_realtimeSender.Reconnect();
  if (!m_wasConnected)
    return false;

  m_wasConnected = m_realtimeSender.Send(m_points);
  return m_wasConnected;
}
}  // namespace tracking

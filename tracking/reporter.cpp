#include "reporter.hpp"

#include "platform/location.hpp"
#include "platform/socket.hpp"

#include "base/logging.hpp"
#include "base/timer.hpp"

#include "std/target_os.hpp"

namespace
{
double constexpr kRequiredHorizontalAccuracy = 10.0;
double constexpr kMinDelaySeconds = 1.0;
double constexpr kReconnectDelaySeconds = 60.0;
size_t constexpr kRealTimeBufferSize = 60;
} // namespace

namespace tracking
{
const char Reporter::kEnableTrackingKey[] = "StatisticsEnabled";

// static
milliseconds const Reporter::kPushDelayMs = milliseconds(20000);

Reporter::Reporter(unique_ptr<platform::Socket> socket, string const & host, uint16_t port,
                   milliseconds pushDelay)
  : m_allowSendingPoints(true)
  , m_realtimeSender(move(socket), host, port, false)
  , m_pushDelay(pushDelay)
  , m_points(kRealTimeBufferSize)
  , m_thread([this] { Run(); })
{
}

Reporter::~Reporter()
{
  {
    lock_guard<mutex> lg(m_mutex);
    m_isFinished = true;
  }
  m_cv.notify_one();
  m_thread.join();
}

void Reporter::AddLocation(location::GpsInfo const & info, traffic::SpeedGroup /* speedGroup */)
{
  lock_guard<mutex> lg(m_mutex);

  if (info.m_horizontalAccuracy > kRequiredHorizontalAccuracy)
    return;

  if (info.m_timestamp < m_lastGpsTime + kMinDelaySeconds)
    return;

  m_lastGpsTime = info.m_timestamp;
  m_input.push_back(DataPoint(info.m_timestamp, ms::LatLon(info.m_latitude, info.m_longitude)));
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
    {
      m_idleFn();
    }
    else
    {
      if (SendPoints())
        m_points.clear();
    }
    lock.lock();

    auto const passedMs = duration_cast<milliseconds>(steady_clock::now() - startTime);
    if (passedMs < m_pushDelay)
      m_cv.wait_for(lock, m_pushDelay - passedMs, [this]{return m_isFinished;});
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

  double const currentTime = my::Timer::LocalTime();
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

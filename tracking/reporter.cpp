#include "reporter.hpp"

#include "base/logging.hpp"
#include "base/thread.hpp"
#include "base/timer.hpp"

#include "boost/circular_buffer.hpp"

#include "platform/location.hpp"
#include "platform/socket.hpp"

#include "std/mutex.hpp"
#include "std/vector.hpp"

#include "tracking/connection.hpp"

using namespace tracking;

namespace
{
double constexpr kMinDelaySeconds = 1.0;
double constexpr kReconnectDelaySeconds = 60.0;
size_t constexpr kRealTimeBufferSize = 60;

class WorkerImpl final : public Reporter::Worker
{
public:
  WorkerImpl(unique_ptr<platform::Socket> socket, size_t pushDelayMs);
  void Run();

  // Worker overrides
  void AddLocation(location::GpsInfo const & info);
  void Stop();

private:
  void FetchInput();
  bool SendPoints();

  volatile bool m_stop = false;
  Connection m_realtimeSender;
  size_t m_pushDelayMs;
  bool m_wasConnected = false;
  double m_lastConnectTryTime = 0.0;
  vector<tracking::DataPoint> m_input;
  mutex m_inputMutex;
  boost::circular_buffer<DataPoint> m_points;
  double m_lastGpsTime = 0.0;
};
}  // namespace

namespace tracking
{
// static
const char Reporter::kAllowKey[] = "AllowStat";

Reporter::Reporter(unique_ptr<platform::Socket> socket, size_t pushDelayMs)
{
  WorkerImpl * worker = new WorkerImpl(move(socket), pushDelayMs);
  m_worker = worker;
  threads::SimpleThread thread([worker]
  {
    worker->Run();
    delete worker;
  });
  thread.detach();
}

Reporter::~Reporter() { m_worker->Stop(); }

void Reporter::AddLocation(location::GpsInfo const & info) { m_worker->AddLocation(info); }
}  // namespace tracking

namespace
{
WorkerImpl::WorkerImpl(unique_ptr<platform::Socket> socket, size_t pushDelayMs)
  : m_realtimeSender(move(socket), Connection::kHost, Connection::kPort, false)
  , m_pushDelayMs(pushDelayMs)
  , m_points(kRealTimeBufferSize)
{
}

void WorkerImpl::AddLocation(location::GpsInfo const & info)
{
  lock_guard<mutex> lg(m_inputMutex);

  if (info.m_timestamp < m_lastGpsTime + kMinDelaySeconds)
    return;

  m_lastGpsTime = info.m_timestamp;
  m_input.push_back(DataPoint(info.m_timestamp, ms::LatLon(info.m_latitude, info.m_longitude)));
}

void WorkerImpl::Run()
{
  LOG(LINFO, ("Tracking Reporter started"));

  my::HighResTimer timer;

  while (!m_stop)
  {
    uint64_t const startMs = timer.ElapsedMillis();

    FetchInput();
    if (SendPoints())
      m_points.clear();

    uint64_t const passedMs = timer.ElapsedMillis() - startMs;
    if (passedMs < m_pushDelayMs)
      threads::Sleep(m_pushDelayMs - passedMs);
  }

  LOG(LINFO, ("Tracking Reporter finished"));
}

void WorkerImpl::Stop() { m_stop = true; }
void WorkerImpl::FetchInput()
{
  lock_guard<mutex> lg(m_inputMutex);
  m_points.insert(m_points.end(), m_input.begin(), m_input.end());
  m_input.clear();
}

bool WorkerImpl::SendPoints()
{
  if (m_points.empty())
    return true;

  if (m_wasConnected)
    m_wasConnected = m_realtimeSender.Send(m_points);

  if (m_wasConnected)
    return true;

  double currentTime = my::Timer::LocalTime();
  if (currentTime < m_lastConnectTryTime + kReconnectDelaySeconds)
    return false;

  m_lastConnectTryTime = currentTime;

  m_wasConnected = m_realtimeSender.Reconnect();
  if (!m_wasConnected)
    return false;

  m_wasConnected = m_realtimeSender.Send(m_points);
  return m_wasConnected;
}
}  // namespace

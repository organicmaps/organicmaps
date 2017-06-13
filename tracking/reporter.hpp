#pragma once

#include "tracking/connection.hpp"

#include "traffic/speed_groups.hpp"

#include "base/thread.hpp"

#include "std/atomic.hpp"
#include "std/chrono.hpp"
#include "std/condition_variable.hpp"
#include "std/function.hpp"
#include "std/mutex.hpp"
#include "std/string.hpp"
#include "std/unique_ptr.hpp"
#include "std/vector.hpp"

#include "boost/circular_buffer.hpp"

namespace location
{
class GpsInfo;
}

namespace platform
{
class Socket;
}

namespace tracking
{
class Reporter final
{
public:
  static milliseconds const kPushDelayMs;
  static const char kEnableTrackingKey[];

  Reporter(unique_ptr<platform::Socket> socket, string const & host, uint16_t port,
           milliseconds pushDelay);
  ~Reporter();

  void AddLocation(location::GpsInfo const & info, traffic::SpeedGroup traffic);

  void SetAllowSendingPoints(bool allow) { m_allowSendingPoints = allow; }

  inline void SetIdleFunc(function<void()> fn) { m_idleFn = fn; }

private:
  void Run();
  bool SendPoints();

  atomic<bool> m_allowSendingPoints;
  Connection m_realtimeSender;
  milliseconds m_pushDelay;
  bool m_wasConnected = false;
  double m_lastConnectionAttempt = 0.0;
  double m_lastNotChargingEvent = 0.0;
  // Function to be called every |kPushDelayMs| in
  // case no points were sent.
  function<void()> m_idleFn;
  // Input buffer for incoming points. Worker thread steals it contents.
  vector<DataPoint> m_input;
  // Last collected points, sends periodically to server.
  boost::circular_buffer<DataPoint> m_points;
  double m_lastGpsTime = 0.0;
  bool m_isFinished = false;
  mutex m_mutex;
  condition_variable m_cv;
  threads::SimpleThread m_thread;
};
}  // namespace tracking

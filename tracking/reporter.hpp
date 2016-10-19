#pragma once

#include "tracking/connection.hpp"

#include "base/thread.hpp"

#include "std/chrono.hpp"
#include "std/condition_variable.hpp"
#include "std/mutex.hpp"
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
  static const char kEnabledSettingsKey[];

  Reporter(unique_ptr<platform::Socket> socket, milliseconds pushDelay);
  ~Reporter();

  void AddLocation(location::GpsInfo const & info);

private:
  void Run();
  bool SendPoints();

  Connection m_realtimeSender;
  milliseconds m_pushDelay;
  bool m_wasConnected = false;
  double m_lastConnectionAttempt = 0.0;
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

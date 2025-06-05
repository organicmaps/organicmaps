#pragma once

#include "tracking/connection.hpp"

#include "traffic/speed_groups.hpp"

#include "base/thread.hpp"

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-copy"
#endif
#include <boost/circular_buffer.hpp>
#ifdef __clang__
#pragma clang diagnostic pop
#endif

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
  static std::chrono::milliseconds const kPushDelayMs;
  static char const kEnableTrackingKey[];

  Reporter(std::unique_ptr<platform::Socket> socket, std::string const & host, uint16_t port,
           std::chrono::milliseconds pushDelay);
  ~Reporter();

  void AddLocation(location::GpsInfo const & info, traffic::SpeedGroup traffic);

  void SetAllowSendingPoints(bool allow) { m_allowSendingPoints = allow; }

  void SetIdleFunc(std::function<void()> fn) { m_idleFn = fn; }

private:
  void Run();
  bool SendPoints();

  std::atomic<bool> m_allowSendingPoints;
  Connection m_realtimeSender;
  std::chrono::milliseconds m_pushDelay;
  bool m_wasConnected = false;
  double m_lastConnectionAttempt = 0.0;
  double m_lastNotChargingEvent = 0.0;
  // Function to be called every |kPushDelayMs| in
  // case no points were sent.
  std::function<void()> m_idleFn;
  // Input buffer for incoming points. Worker thread steals it contents.
  std::vector<DataPoint> m_input;
  // Last collected points, sends periodically to server.
  boost::circular_buffer<DataPoint> m_points;
  double m_lastGpsTime = 0.0;
  bool m_isFinished = false;
  std::mutex m_mutex;
  std::condition_variable m_cv;
  threads::SimpleThread m_thread;
};
}  // namespace tracking

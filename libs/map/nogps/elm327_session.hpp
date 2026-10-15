#pragma once

#include "map/nogps/motion_source.hpp"
#include "map/nogps/scheduler.hpp"

#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>

namespace nogps
{
class Clock;
class Delegate;

/// Reads the car speed from an ELM327 OBD-II adapter: initializes it and asks the speed again and again.
/// The platform connects to the adapter and passes the bytes, reconnections are done on errors.
class Elm327Session
{
public:
  class Listener
  {
  public:
    virtual ~Listener() = default;
    /// \param timeMs monotonic time the speed was measured at.
    virtual void OnSpeed(int speedKmh, int64_t timeMs) = 0;
  };

  static int64_t constexpr kReconnectDelayMs = 3000;
  static int64_t constexpr kResponseTimeoutMs = 3000;
  // Includes the protocol search on the first request.
  static int64_t constexpr kFirstResponseTimeoutMs = 15'000;
  // The adapter replies in ~50-200 ms, don't flood the car bus.
  static int64_t constexpr kRequestIntervalMs = 100;
  // Many adapters don't find the car again after its ignition was off: the adapter is reset then.
  static int constexpr kMaxNoDataRequests = 3;

  Elm327Session(Delegate & delegate, Scheduler & scheduler, Clock const & clock, std::string address,
                Listener & listener);
  ~Elm327Session();

  void Start();
  void Stop();

  void OnConnected();
  void OnBytes(std::string_view data);
  void OnClosed(std::string const & reason);

  /// \returns Disconnected, Connecting, NoAdapter, NoCarData or Connected.
  SourceState GetState() const { return m_state; }

private:
  enum class Step
  {
    Idle,
    Connecting,
    Initializing,
    Polling,
  };

  void Connect();
  void SendCommand(std::string const & command, int64_t timeoutMs);
  void OnResponse(std::string const & response);
  void RequestSpeed();
  void Fail(std::string const & reason);
  void SetState(SourceState state);

  Delegate & m_delegate;
  Clock const & m_clock;
  std::string const m_address;
  Listener & m_listener;
  bool m_running = false;
  Step m_step = Step::Idle;
  SourceState m_state = SourceState::Disconnected;
  // The state was set at least once: the first one is always logged.
  bool m_hasState = false;
  // The bytes after the last response.
  std::string m_received;
  std::string m_command;
  size_t m_initCommand = 0;
  int64_t m_requestTimeMs = 0;
  int64_t m_pollTimeoutMs = kFirstResponseTimeoutMs;
  int m_noDataRequests = 0;
  // The response to the command must come before it.
  Timer m_timeout;
  // The next speed request or reconnection.
  Timer m_next;
};
}  // namespace nogps

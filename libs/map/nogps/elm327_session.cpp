#include "map/nogps/elm327_session.hpp"

#include "map/nogps/clock.hpp"
#include "map/nogps/delegate.hpp"
#include "map/nogps/elm327_parser.hpp"

#include "base/logging.hpp"

#include <array>

namespace nogps
{
namespace
{
std::array<char const *, 6> constexpr kElm327InitCommands = {
    "ATZ",    // Reset.
    "ATE0",   // Echo off.
    "ATL0",   // Linefeeds off.
    "ATS0",   // Spaces off.
    "ATH0",   // Headers off.
    "ATSP0",  // Automatic protocol.
};
}  // namespace

Elm327Session::Elm327Session(Delegate & delegate, Scheduler & scheduler, Clock const & clock, std::string address,
                             Listener & listener)
  : m_delegate(delegate)
  , m_clock(clock)
  , m_address(std::move(address))
  , m_listener(listener)
  , m_timeout(scheduler)
  , m_next(scheduler)
{}

Elm327Session::~Elm327Session()
{
  Stop();
}

void Elm327Session::Start()
{
  if (m_running)
    return;
  m_running = true;
  Connect();
}

void Elm327Session::Stop()
{
  if (!m_running)
    return;
  m_running = false;
  m_step = Step::Idle;
  m_timeout.Stop();
  m_next.Stop();
  m_delegate.Elm327Close();
  SetState(SourceState::Disconnected);
}

void Elm327Session::Connect()
{
  // While the adapter doesn't answer, the reconnections don't blink with "connecting".
  if (m_state != SourceState::NoAdapter)
    SetState(SourceState::Connecting);
  m_step = Step::Connecting;
  m_delegate.Elm327Connect(m_address);
}

void Elm327Session::OnConnected()
{
  if (!m_running || m_step != Step::Connecting)
    return;
  LOG(LINFO, ("Connected to", m_address));
  m_step = Step::Initializing;
  m_initCommand = 0;
  SendCommand(kElm327InitCommands[0], kResponseTimeoutMs);
}

void Elm327Session::OnBytes(std::string_view data)
{
  if (!m_running || (m_step != Step::Initializing && m_step != Step::Polling))
    return;
  m_received.append(data);
  // The adapter ends every response with the '>' prompt.
  auto const prompt = m_received.find('>');
  if (prompt == std::string::npos || m_command.empty())
    return;
  std::string const response = m_received.substr(0, prompt);
  m_received.erase(0, prompt + 1);
  m_timeout.Stop();
  m_command.clear();
  OnResponse(response);
}

void Elm327Session::OnClosed(std::string const & reason)
{
  if (m_running && m_step != Step::Idle)
    Fail(reason);
}

void Elm327Session::SendCommand(std::string const & command, int64_t timeoutMs)
{
  m_command = command;
  m_received.clear();
  m_requestTimeMs = m_clock.NowMs();
  m_timeout.Start(timeoutMs, [this] { Fail("Timeout on " + m_command); });
  m_delegate.Elm327Write(command + "\r");
}

void Elm327Session::OnResponse(std::string const & response)
{
  if (m_step == Step::Initializing)
  {
    if (++m_initCommand < kElm327InitCommands.size())
    {
      SendCommand(kElm327InitCommands[m_initCommand], kResponseTimeoutMs);
      return;
    }
    SetState(SourceState::NoCarData);
    m_step = Step::Polling;
    m_pollTimeoutMs = kFirstResponseTimeoutMs;
    m_noDataRequests = 0;
    RequestSpeed();
    return;
  }

  int64_t const timeMs = (m_requestTimeMs + m_clock.NowMs()) / 2;
  auto const speed = ParseElm327Speed(response);
  if (!speed)
  {
    SetState(SourceState::NoCarData);
    if (++m_noDataRequests >= kMaxNoDataRequests)
    {
      Fail("No speed from the car: " + response);
      return;
    }
    m_pollTimeoutMs = kFirstResponseTimeoutMs;
    m_next.Start(kReconnectDelayMs, [this] { RequestSpeed(); });
    return;
  }
  m_noDataRequests = 0;
  m_pollTimeoutMs = kResponseTimeoutMs;
  SetState(SourceState::Connected);
  m_listener.OnSpeed(*speed, timeMs);
  m_next.Start(kRequestIntervalMs, [this] { RequestSpeed(); });
}

void Elm327Session::RequestSpeed()
{
  SendCommand("010D", m_pollTimeoutMs);
}

void Elm327Session::Fail(std::string const & reason)
{
  LOG(LWARNING, ("ELM327 error:", reason));
  m_timeout.Stop();
  m_next.Stop();
  m_command.clear();
  m_step = Step::Idle;
  m_delegate.Elm327Close();
  // The adapter has never answered the initialization since the connection started.
  bool const noAdapter = m_state == SourceState::Connecting || m_state == SourceState::NoAdapter;
  SetState(noAdapter ? SourceState::NoAdapter : SourceState::Disconnected);
  m_next.Start(kReconnectDelayMs, [this] { Connect(); });
}

void Elm327Session::SetState(SourceState state)
{
  if (m_hasState && state == m_state)
    return;
  m_hasState = true;
  m_state = state;
  LOG(LINFO, ("ELM327 state =", state));
}
}  // namespace nogps

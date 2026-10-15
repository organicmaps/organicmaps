#include "map/nogps/esp32_update.hpp"

#include "map/nogps/clock.hpp"

#include "base/assert.hpp"
#include "base/logging.hpp"
#include "base/string_utils.hpp"

#include <algorithm>

#include <boost/hash2/sha2.hpp>

namespace nogps
{
namespace
{
std::string Sha256Hex(std::string const & data)
{
  boost::hash2::sha2_256 hash;
  hash.update(data.data(), data.size());
  char constexpr kHex[] = "0123456789abcdef";
  std::string hex;
  for (unsigned char const byte : hash.result())
  {
    hex += kHex[byte >> 4];
    hex += kHex[byte & 15];
  }
  return hex;
}
}  // namespace

Esp32Update::Esp32Update(Link & link, Scheduler & scheduler, Clock const & clock)
  : m_link(link)
  , m_clock(clock)
  , m_checkTimer(scheduler)
{}

void Esp32Update::Start(std::string image, std::string const & version)
{
  Reset();
  m_image = std::move(image);
  m_version = version;
  m_size = m_image.size();
  if (m_image.empty())
  {
    Fail("NO_FILE");
    return;
  }
  LOG(LINFO, ("Firmware", m_version, "of", m_size, "bytes"));
  SetState(State::Starting);
  m_commandId =
      m_link.SendUpdateCommand("OTA_BEGIN," + std::to_string(m_size) + "," + Sha256Hex(m_image) + "," + m_version);
  StartCheckTimer();
}

void Esp32Update::Cancel()
{
  if (!IsActive())
    return;
  // The box restarts by itself after it has checked the firmware, it can't be stopped then.
  if (m_state != State::Restarting)
    m_link.SendUpdateCommand("OTA_ABORT");
  Fail("CANCELED");
}

void Esp32Update::Reset()
{
  m_checkTimer.Stop();
  m_state = State::None;
  m_error.clear();
  m_image = {};
  m_version.clear();
  m_commandId = 0;
  m_size = 0;
  m_received = 0;
  m_sent = 0;
  m_linkLost = false;
}

bool Esp32Update::OnReply(esp32::Reply const & reply)
{
  if (m_commandId == 0 || reply.m_id != m_commandId)
    return false;
  if (reply.m_progress)
    return true;
  m_commandId = 0;
  if (!reply.m_ok)
  {
    Fail(reply.m_error.empty() ? "ERR" : reply.m_error);
    return true;
  }
  if (m_state == State::Verifying)
  {
    SetState(State::Restarting);
    return true;
  }
  if (m_state != State::Starting)
    return true;

  // The size of a piece and how far ahead of the received bytes the pieces are taken.
  uint32_t pieceSize = 0;
  uint32_t window = 0;
  if (reply.m_values.size() < 2 || !strings::to_uint(reply.m_values[0], pieceSize) ||
      !strings::to_uint(reply.m_values[1], window) || pieceSize == 0 || window < pieceSize)
  {
    m_link.SendUpdateCommand("OTA_ABORT");
    Fail("VALUE");
    return true;
  }
  m_pieceSize = pieceSize;
  m_window = window;
  m_receivedGrewMs = m_clock.NowMs();
  m_resentMs = m_receivedGrewMs;
  SetState(State::Sending);
  SendPieces();
  return true;
}

void Esp32Update::OnProgress(esp32::UpdateProgress const & progress)
{
  if (m_state != State::Sending && m_state != State::Verifying)
    return;
  auto const & state = progress.m_state;
  if (state.starts_with("ERR_"))
  {
    Fail(state.substr(4));
    return;
  }
  if (state == "ABORTED")
  {
    Fail(state);
    return;
  }
  if (state == "DONE")
  {
    SetState(State::Restarting);
    return;
  }
  if (m_state != State::Sending)
    return;

  size_t const received = std::min<size_t>(progress.m_received, m_size);
  if (received > m_received)
  {
    m_received = received;
    m_receivedGrewMs = m_clock.NowMs();
  }
  // A piece was lost: the box has dropped everything after it.
  if (state == "RESEND")
  {
    m_received = received;
    m_sent = received;
  }
  if (m_received == m_size)
  {
    SetState(State::Verifying);
    m_commandId = m_link.SendUpdateCommand("OTA_END");
    return;
  }
  SendPieces();
}

void Esp32Update::OnLinkLost()
{
  // The box restarts a second after it has taken the firmware: its reply may be lost with the connection.
  if (m_state == State::Verifying)
    SetState(State::Restarting);
  if (m_state == State::Restarting)
    m_linkLost = true;
  else if (IsActive())
    Fail("LINK");
}

void Esp32Update::OnBoxFirmware(std::string const & version)
{
  if (m_state != State::Restarting || !m_linkLost)
    return;
  // The box has come back with its previous firmware: the new one didn't start or was not accepted.
  if (version != m_version)
  {
    Fail("ROLLBACK");
    return;
  }
  LOG(LINFO, ("The box works with the firmware", version));
  m_checkTimer.Stop();
  m_image = {};
  SetState(State::Done);
}

int Esp32Update::GetProgressPercent() const
{
  switch (m_state)
  {
  case State::Sending: return static_cast<int>(m_received * 100 / m_size);
  case State::Verifying:
  case State::Restarting:
  case State::Done: return 100;
  case State::None:
  case State::Starting:
  case State::Failed: return 0;
  }
  UNREACHABLE();
}

void Esp32Update::SendPieces()
{
  while (m_sent < m_size)
  {
    size_t const size = std::min(m_pieceSize, m_size - m_sent);
    if (m_sent + size > m_received + m_window)
      break;
    std::string piece(4, '\0');
    for (size_t i = 0; i < 4; ++i)
      piece[i] = static_cast<char>((m_sent >> (8 * i)) & 0xFF);
    piece.append(m_image, m_sent, size);
    m_link.SendFirmwarePiece(piece);
    m_sent += size;
  }
}

void Esp32Update::Fail(std::string const & error)
{
  LOG(LWARNING, ("The firmware update has failed:", error, "state =", m_state, "received =", m_received, "of", m_size));
  m_checkTimer.Stop();
  m_commandId = 0;
  m_image = {};
  m_error = error;
  m_state = State::Failed;
}

void Esp32Update::SetState(State state)
{
  LOG(LINFO, ("Firmware update:", state));
  m_state = state;
  m_stateSinceMs = m_clock.NowMs();
}

void Esp32Update::StartCheckTimer()
{
  m_checkTimer.Start(kCheckIntervalMs, [this] { Check(); });
}

void Esp32Update::Check()
{
  int64_t const now = m_clock.NowMs();
  int64_t const inState = now - m_stateSinceMs;
  switch (m_state)
  {
  case State::Starting:
    if (inState >= kReplyTimeoutMs)
      Fail("NO_REPLY");
    break;
  case State::Sending:
    if (now - m_receivedGrewMs >= kStallMs)
    {
      m_link.SendUpdateCommand("OTA_ABORT");
      Fail("TIMEOUT");
    }
    else if (m_sent > m_received && now - std::max(m_receivedGrewMs, m_resentMs) >= kResendMs)
    {
      LOG(LINFO, ("The box stays at", m_received, "sending again"));
      m_resentMs = now;
      m_sent = m_received;
      SendPieces();
    }
    break;
  case State::Verifying:
    if (inState >= kVerifyTimeoutMs)
      Fail("NO_REPLY");
    break;
  case State::Restarting:
    if (inState >= kRestartTimeoutMs)
      Fail("NO_RESTART");
    break;
  case State::None:
  case State::Done:
  case State::Failed: break;
  }
  if (IsActive())
    StartCheckTimer();
}

std::string DebugPrint(Esp32Update::State state)
{
  switch (state)
  {
  case Esp32Update::State::None: return "NONE";
  case Esp32Update::State::Starting: return "STARTING";
  case Esp32Update::State::Sending: return "SENDING";
  case Esp32Update::State::Verifying: return "VERIFYING";
  case Esp32Update::State::Restarting: return "RESTARTING";
  case Esp32Update::State::Done: return "DONE";
  case Esp32Update::State::Failed: return "FAILED";
  }
  UNREACHABLE();
}
}  // namespace nogps

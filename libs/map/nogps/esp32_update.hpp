#pragma once

#include "map/nogps/esp32_protocol.hpp"
#include "map/nogps/scheduler.hpp"

#include <cstdint>
#include <functional>
#include <string>

namespace nogps
{
class Clock;

/// A firmware of the ESP32 sensor box that comes with the application: the one the application is tested with.
struct Firmware
{
  std::string m_version;
  // The box takes the firmware made for its chip and board only.
  std::string m_chip;
  std::string m_board;
  // The file as it is sent to the box, with its signature. Read when it is needed: it is over a megabyte.
  std::function<std::string()> m_read;
};

/// Sends a firmware to the ESP32 sensor box over Bluetooth LE, see docs/nogps/esp32-firmware-spec.md, section 18.
/// The box writes it next to the running one, checks its hash and signature and restarts with it. Until then the
/// running firmware works whatever happens, so a failed update is just started again.
class Esp32Update
{
public:
  enum class State
  {
    None,
    // The box is asked to take the firmware.
    Starting,
    Sending,
    // Everything is sent, the box checks it.
    Verifying,
    // The box restarts with the new firmware and tells its version.
    Restarting,
    Done,
    Failed,
  };

  /// How the update reaches the box.
  class Link
  {
  public:
    virtual ~Link() = default;
    /// \returns the id of the sent command, its reply comes to OnReply().
    virtual int SendUpdateCommand(std::string const & command) = 0;
    /// \param piece the offset in the firmware, 4 bytes, the lowest first, and the bytes from it.
    virtual void SendFirmwarePiece(std::string const & piece) = 0;
  };

  // The box answers the commands at once.
  static int64_t constexpr kReplyTimeoutMs = 5000;
  // The pieces are written without a response and may be lost: when the box has received nothing new for this
  // long, they are sent again from where it has stopped.
  static int64_t constexpr kResendMs = 2000;
  // The box itself gives the update up after 30 s without pieces.
  static int64_t constexpr kStallMs = 20'000;
  // The hash and the signature of a megabyte are checked for seconds.
  static int64_t constexpr kVerifyTimeoutMs = 30'000;
  // The restart of the box, the search and the connection.
  static int64_t constexpr kRestartTimeoutMs = 60'000;
  static int64_t constexpr kCheckIntervalMs = 1000;

  Esp32Update(Link & link, Scheduler & scheduler, Clock const & clock);

  /// \param image the file of the firmware.
  /// \param version the version the box tells after it has restarted with the firmware.
  void Start(std::string image, std::string const & version);
  /// Stops the update by the user: the box keeps its firmware.
  void Cancel();
  /// Forgets the update, e.g. the box is not used any more.
  void Reset();

  /// \returns true if the reply is to a command of the update.
  bool OnReply(esp32::Reply const & reply);
  void OnProgress(esp32::UpdateProgress const & progress);
  /// The connection to the box is lost: an update is not continued, but a restarting box is waited for.
  void OnLinkLost();
  /// The box tells its firmware when a connection starts.
  void OnBoxFirmware(std::string const & version);

  State GetState() const { return m_state; }
  /// \returns true from the start until the box works with the new firmware or the update fails.
  bool IsActive() const { return m_state != State::None && m_state != State::Done && m_state != State::Failed; }
  int GetProgressPercent() const;
  /// \returns why the update has failed: a code of the box (MOVING, LOW_VOLTAGE, SIGNATURE etc.) or LINK,
  /// NO_REPLY, TIMEOUT, NO_RESTART, ROLLBACK, CANCELED.
  std::string const & GetError() const { return m_error; }

private:
  void SendPieces();
  void Fail(std::string const & error);
  void Check();
  void StartCheckTimer();
  void SetState(State state);

  Link & m_link;
  Clock const & m_clock;
  Timer m_checkTimer;

  State m_state = State::None;
  std::string m_error;
  // Dropped when the update ends.
  std::string m_image;
  size_t m_size = 0;
  std::string m_version;
  // The command waiting for its reply, 0 if none.
  int m_commandId = 0;
  int64_t m_stateSinceMs = 0;
  // The size of a piece and how far ahead of the received bytes the box takes them, told by the box.
  size_t m_pieceSize = 0;
  size_t m_window = 0;
  // The bytes the box has written in a row from the start, and the bytes sent.
  size_t m_received = 0;
  size_t m_sent = 0;
  int64_t m_receivedGrewMs = 0;
  int64_t m_resentMs = 0;
  // The box has restarted: its firmware told before that is the old one.
  bool m_linkLost = false;
};

std::string DebugPrint(Esp32Update::State state);
}  // namespace nogps

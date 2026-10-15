#pragma once

#include "geometry/latlon.hpp"

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace nogps
{
/// Detects jammed/spoofed GNSS positions (e.g. electronic warfare moves the phone to another continent).
/// A satellite position is not trusted when it contradicts the network (cell towers and Wi-Fi) position,
/// or when it jumps farther than any car could drive since the last trusted satellite position.
/// After a detection, satellite positions are trusted again only after several consecutive consistent ones.
/// All the times are monotonic, e.g. elapsed realtime.
class GpsSpoofingDetector
{
public:
  // Network positions older than this are not used to verify satellite positions.
  static int64_t constexpr kNetworkMaxAgeMs = 3 * 60 * 1000;
  // Network positions are coarse (up to several km in the countryside), so smaller mismatches are ignored.
  static double constexpr kNetworkMinToleranceM = 3000;
  static double constexpr kNetworkAccuracyFactor = 3;
  // Faster than any car on Ukrainian roads, used to estimate how far the user could move between positions.
  static double constexpr kMaxSpeedMps = 300 / 3.6;
  // Jumps shorter than this are GPS noise rather than spoofing.
  static double constexpr kMinJumpM = 1000;
  static int constexpr kConsistentPositionsToRecover = 5;
  // Without the network only the last trusted position is available, so be more careful.
  static int constexpr kConsistentPositionsToRecoverWithoutNetwork = 10;
  // A network position is sometimes kilometers away, e.g. from a Wi-Fi point that has moved or a cell tower with a
  // wrong location in the database. Such a cell tower gives the same point again and again, so a jump is used only
  // when a network position at another point near it confirms it, or when nothing contradicts it for this long.
  static int64_t constexpr kJumpConfirmMs = kNetworkMaxAgeMs;
  // Positions from the same cell tower or Wi-Fi point.
  static double constexpr kSamePointM = 10;
  // Points that jumped away and back are ignored. They are saved, as after a restart such a point may be the only
  // one.
  static size_t constexpr kMaxPhantoms = 16;
  // A Wi-Fi position (tens of meters) is used at once after a coarse cell tower one (hundreds of meters).
  static double constexpr kMoreAccurateFactor = 2;

  struct Trusted
  {
    ms::LatLon m_position;
    int64_t m_timeMs = 0;
  };

  bool IsSpoofed() const { return m_spoofed; }

  /// \returns the wrong network points as "lat,lon;lat,lon".
  std::string SerializePhantoms() const;
  void DeserializePhantoms(std::string_view phantoms);
  size_t GetPhantomsCount() const { return m_phantoms.size(); }

  /// \returns the last trusted satellite or network position.
  std::optional<Trusted> GetLastTrusted() const;
  /// Restores the position trusted before the app restarted, as after a restart a spoofed position may come first.
  /// \param timeMs may be negative if it was before the device restart.
  void RestoreTrusted(ms::LatLon const & position, int64_t timeMs);

  /// \returns false if the position jumped away and is ignored.
  bool OnNetworkPosition(ms::LatLon const & position, double accuracyM, int64_t timeMs);

  /// Checks a satellite (or fused) position.
  /// \returns true if the position can be trusted.
  bool CheckSatellitePosition(ms::LatLon const & position, int64_t timeMs);

private:
  struct Network
  {
    ms::LatLon m_position;
    double m_accuracyM = 0;
    int64_t m_timeMs = 0;
  };

  struct Jump
  {
    ms::LatLon m_position;
    int64_t m_timeMs = 0;
  };

  void AddPhantom(ms::LatLon const & position);
  bool AcceptNetworkPosition(ms::LatLon const & position, double accuracyM, int64_t timeMs);
  bool IsConsistent(ms::LatLon const & position, int64_t timeMs) const;
  bool HasFreshNetwork(int64_t timeMs) const;
  void SetTrusted(ms::LatLon const & position, int64_t timeMs);

  std::optional<Network> m_network;
  std::optional<Trusted> m_trusted;
  bool m_spoofed = false;
  int m_consistentCount = 0;
  // The first network position of a jump away from the last trusted one, waiting for a confirmation.
  std::optional<Jump> m_jump;
  std::vector<ms::LatLon> m_phantoms;
  // Several providers report the same network position.
  std::optional<int64_t> m_lastNetworkTimeMs;
  bool m_lastNetworkAccepted = false;
};
}  // namespace nogps

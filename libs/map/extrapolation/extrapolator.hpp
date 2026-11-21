#pragma once

#include "platform/location.hpp"

#include <cstdint>
#include <functional>
#include <mutex>

namespace extrapolation
{
enum class UpdateType
{
  Measurement,
  // Display tick between GPS fixes. It may repeat the last coordinates.
  Prediction
};

/// \brief Returns extrapolated position after |point2| in |timeAfterPoint2Ms|.
/// \note |timeAfterPoint2Ms| should be reasonably small (several seconds maximum).
location::GpsInfo LinearExtrapolation(location::GpsInfo const & gpsInfo1, location::GpsInfo const & gpsInfo2,
                                      uint64_t timeAfterPoint2Ms);

/// \returns true if linear extrapolation based on |info1| and |info2| should be done.
/// \param info1 location information about point gotten before last one.
/// \param info2 the latest location information.
bool AreCoordsGoodForExtrapolation(location::GpsInfo const & info1, location::GpsInfo const & info2);

// Delivers each current GPS fix and, while enabled, predictions based on the last two fixes.
class Extrapolator
{
public:
  using LocationUpdateFn = std::function<void(location::GpsInfo const &, UpdateType)>;

  // Extrapolate every 200 ms for up to two seconds after the latest fix.
  static uint64_t constexpr kMaxExtrapolationTimeMs = 2000;
  static uint64_t constexpr kExtrapolationPeriodMs = 200;

  // The GUI thread owns location input and delivery. Predictions are computed on the background thread.
  explicit Extrapolator(LocationUpdateFn update);
  void OnLocationUpdate(location::GpsInfo const & gpsInfo);
  void Enable(bool enabled);

private:
  void ProcessLocationTick(uint64_t generation);
  void RunTaskOnBackgroundThread(bool delayed, uint64_t generation);

  std::mutex m_mutex;
  LocationUpdateFn m_update;
  location::GpsInfo m_lastGpsInfo;
  location::GpsInfo m_beforeLastGpsInfo;
  uint64_t m_generation = 0;
  uint64_t m_consecutiveRuns = 0;
  bool m_isEnabled = false;
};
}  // namespace extrapolation

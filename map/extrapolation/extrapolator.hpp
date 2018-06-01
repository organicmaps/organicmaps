#pragma once

#include "platform/location.hpp"

#include <cstdint>
#include <functional>
#include <limits>
#include <mutex>

namespace extrapolation
{
/// \brief Returns extrapolated position after |point2| in |timeAfterPoint2Ms|.
/// \note |timeAfterPoint2Ms| should be relevantly small (several seconds maximum).
location::GpsInfo LinearExtrapolation(location::GpsInfo const & gpsInfo1,
                                      location::GpsInfo const & gpsInfo2,
                                      uint64_t timeAfterPoint2Ms);

bool AreCoordsGoodForExtrapolation(location::GpsInfo const & beforeLastGpsInfo,
                                   location::GpsInfo const & lastGpsInfo);

class Extrapolator
{
  static uint64_t constexpr kExtrapolationCounterUndefined = std::numeric_limits<uint64_t>::max();

public:
  using ExtrapolatedLocationUpdateFn = std::function<void(location::GpsInfo const &)>;

  // |kMaxExtrapolationTimeMs| is time in milliseconds showing how long location will be
  // extrapolated after last location gotten from GPS.
  static uint64_t constexpr kMaxExtrapolationTimeMs = 1000;
  // |kExtrapolationPeriodMs| is time in milliseconds showing how often location will be
  // extrapolated. So if the last location was gotten from GPS at time X the next location
  // will be emulated by Extrapolator at X + kExtrapolationPeriodMs.
  // Then X + 2 * kExtrapolationPeriodMs and so on till
  // X + n * kExtrapolationPeriodMs <= kMaxExtrapolationTimeMs.
  static uint64_t constexpr kExtrapolationPeriodMs = 200;

  /// \param update is a function which is called with params according to extrapolated position.
  /// |update| will be called on gui thread.
  explicit Extrapolator(ExtrapolatedLocationUpdateFn const & update);
  void OnLocationUpdate(location::GpsInfo const & gpsInfo);
  // @TODO(bykoianko) Gyroscope information should be taken into account as well for calculation
  // extrapolated position.

  void Enable(bool enabled);

private:
  /// \returns true if there's enough information for extrapolation and extrapolation is enabled.
  /// \note This method should be called only when |m_mutex| is locked.
  bool DoesExtrapolationWork() const;
  void ExtrapolatedLocationUpdate();

  bool m_isEnabled;

  std::mutex m_mutex;
  ExtrapolatedLocationUpdateFn m_extrapolatedLocationUpdate;
  location::GpsInfo m_lastGpsInfo;
  location::GpsInfo m_beforeLastGpsInfo;
  uint64_t m_extrapolationCounter = kExtrapolationCounterUndefined;
};
}  // namespace extrapolation

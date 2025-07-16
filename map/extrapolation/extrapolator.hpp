#pragma once

#include "platform/location.hpp"

#include <cstdint>
#include <functional>
#include <limits>
#include <mutex>

namespace extrapolation
{
/// \brief Returns extrapolated position after |point2| in |timeAfterPoint2Ms|.
/// \note |timeAfterPoint2Ms| should be reasonably small (several seconds maximum).
location::GpsInfo LinearExtrapolation(location::GpsInfo const & gpsInfo1, location::GpsInfo const & gpsInfo2,
                                      uint64_t timeAfterPoint2Ms);

/// \returns true if linear extrapolation based on |info1| and |info2| should be done.
/// \param info1 location information about point gotten before last one.
/// \param info2 the latest location information.
bool AreCoordsGoodForExtrapolation(location::GpsInfo const & info1, location::GpsInfo const & info2);

/// \brief This class implements linear extrapolation based on methods LinearExtrapolation()
/// and AreCoordsGoodForExtrapolation(). The idea implemented in this class is
/// - OnLocationUpdate() should be called from gui thread when new data from gps is available.
/// - When OnLocationUpdate() was called twice so that AreCoordsGoodForExtrapolation()
///   returns true, extrapolation for this two location will be launched.
/// - That means all obsolete extrapolation calls in background thread will be canceled by
///   incrementing |m_locationUpdateMinValid|.
/// - Several new extrapolations based on two previous locations will be generated.
class Extrapolator
{
  static uint64_t constexpr kExtrapolationCounterUndefined = std::numeric_limits<uint64_t>::max();

public:
  using ExtrapolatedLocationUpdateFn = std::function<void(location::GpsInfo const &)>;

  // |kMaxExtrapolationTimeMs| is time in milliseconds showing how long location will be
  // extrapolated after last location gotten from GPS.
  static uint64_t constexpr kMaxExtrapolationTimeMs = 2000;
  // |kExtrapolationPeriodMs| is time in milliseconds showing how often location will be
  // extrapolated. So if the last location was obtained from GPS at time X the next location
  // will be emulated by Extrapolator at time X + kExtrapolationPeriodMs.
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
  void ExtrapolatedLocationUpdate(uint64_t locationUpdateCounter);
  void RunTaskOnBackgroundThread(bool delayed);

  bool m_isEnabled;

  std::mutex m_mutex;
  ExtrapolatedLocationUpdateFn m_extrapolatedLocationUpdate;
  location::GpsInfo m_lastGpsInfo;
  location::GpsInfo m_beforeLastGpsInfo;
  uint64_t m_consecutiveRuns = kExtrapolationCounterUndefined;
  // Number of calls Extrapolator::OnLocationUpdate() method. This way |m_locationUpdateCounter|
  // reflects generation of extrapolations. That mean the next gps location is
  // the next generation.
  uint64_t m_locationUpdateCounter = 0;
  // If |m_locationUpdateCounter| < |m_locationUpdateMinValid| when
  // ExtrapolatedLocationUpdate() is called (on background thread)
  // ExtrapolatedLocationUpdate() cancels its execution.
  uint64_t m_locationUpdateMinValid = 0;
};
}  // namespace extrapolation

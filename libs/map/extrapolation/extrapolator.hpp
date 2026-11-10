#pragma once

#include "platform/location.hpp"

#include <chrono>
#include <cstdint>
#include <functional>
#include <memory>

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

/// Extrapolates the two most recent distinct input times.
/// All public methods and location callbacks run on the GUI thread.
class Extrapolator
{
public:
  using ExtrapolatedLocationUpdateFn = std::function<void(location::GpsInfo const &)>;
  using Clock = std::chrono::steady_clock;
  using Task = std::function<void()>;
  using NowFn = std::function<Clock::time_point()>;
  using ScheduleFn = std::function<void(std::chrono::milliseconds, Task)>;

  // Prediction stops this long after the latest distinct input arrives.
  static uint64_t constexpr kMaxExtrapolationTimeMs = 2000;
  // Target interval between prediction updates.
  static uint64_t constexpr kExtrapolationPeriodMs = 200;

  // Delivers received locations immediately and predictions on the GUI thread.
  explicit Extrapolator(ExtrapolatedLocationUpdateFn const & update);
  // The scheduler delivers tasks on the GUI thread after the given delay.
  Extrapolator(ExtrapolatedLocationUpdateFn const & update, NowFn now, ScheduleFn schedule);
  ~Extrapolator();

  Extrapolator(Extrapolator const &) = delete;
  Extrapolator & operator=(Extrapolator const &) = delete;

  void OnLocationUpdate(location::GpsInfo const & gpsInfo);

  void Enable(bool enabled);
  void Reset();

private:
  struct State;
  static void Schedule(std::shared_ptr<State> const & state, uint64_t generation);
  static void Deliver(std::shared_ptr<State> const & state, uint64_t generation);

  std::shared_ptr<State> m_state;
};
}  // namespace extrapolation

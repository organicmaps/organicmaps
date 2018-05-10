#pragma once

#include "platform/location.hpp"

#include "base/thread.hpp"

#include <cstdint>
#include <functional>
#include <limits>
#include <mutex>
#include <thread>

namespace extrapolation
{
/// \brief Returns extrapolated position after |point2| in |timeAfterPoint2Ms|.
/// \note |timeAfterPoint2Ms| should be relevantly small (several seconds maximum).
location::GpsInfo LinearExtrapolation(location::GpsInfo const & gpsInfo1,
                                      location::GpsInfo const & gpsInfo2,
                                      uint64_t timeAfterPoint2Ms);

class Extrapolator
{
public:
  using ExtrapolatedLocationUpdate = std::function<void(location::GpsInfo &)>;

  static uint64_t constexpr m_extrapolationCounterUndefined = std::numeric_limits<uint64_t>::max();

  /// \param update is a function which is called with params according to extrapolated position.
  /// |update| will be called on gui thread.
  explicit Extrapolator(ExtrapolatedLocationUpdate const & update);
  void OnLocationUpdate(location::GpsInfo const & info);
  // @TODO(bykoianko) Gyroscope information should be taken into account as well for calculation
  // extrapolated position.

  void Cancel() { m_extrapolatedLocationThread.Cancel(); }

private:
  class Routine : public threads::IRoutine
  {
  public:
    explicit Routine(ExtrapolatedLocationUpdate const & update);

    // threads::IRoutine overrides:
    void Do() override;

    void SetGpsInfo(location::GpsInfo const & gpsInfo);

  private:
    bool DoesExtrapolationWork(uint64_t extrapolationTimeMs) const;

    ExtrapolatedLocationUpdate m_extrapolatedLocationUpdate;

    std::mutex m_mutex;
    location::GpsInfo m_lastGpsInfo;
    location::GpsInfo m_beforeLastGpsInfo;
    uint64_t m_extrapolationCounter = m_extrapolationCounterUndefined;
  };

  threads::Thread m_extrapolatedLocationThread;
};
}  // namespace extrapolation

#pragma once

#include "storage/country_info_getter.hpp"
#include "storage/storage_defines.hpp"

#include "geometry/mercator.hpp"
#include "geometry/point2d.hpp"

#include <atomic>
#include <functional>

/// Tracks which country the user's GPS position is in.
class LocationCountryTracker
{
  // Precalculated value that's needed during the runtime check.
  // The value is based on minimal distance in meters
  static double constexpr kDistPrecalculated = []
  {
    double constexpr minDistanceM = 500.0;
    double constexpr distMer = mercator::MetersToMercator(minDistanceM);
    return distMer * distMer;
  }();

public:
  using TCountryChangedFn = std::function<void(storage::CountryId const &)>;

  void SetInfoGetter(storage::CountryInfoGetter const & infoGetter) { m_infoGetter = &infoGetter; }
  void SetListener(TCountryChangedFn listener);
  void OnLocationUpdate(m2::PointD const & position);

  storage::CountryId GetCountryId() const { return m_currentCountry; }

private:
  void RunQuery(m2::PointD const & position);

  storage::CountryInfoGetter const * m_infoGetter = nullptr;
  TCountryChangedFn m_listener;

  storage::CountryId m_currentCountry = storage::kInvalidCountryId;
  m2::PointD m_lastCheckedPos = m2::PointD::Zero();
  m2::RectD m_currentCountryRect;

  std::atomic<bool> m_queryInFlight{false};
};

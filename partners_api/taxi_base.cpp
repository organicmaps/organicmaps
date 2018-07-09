#include "partners_api/taxi_base.hpp"

#include "geometry/distance_on_sphere.hpp"

namespace
{
// The maximum supported distance in meters by default.
double const kMaxSupportedDistance = 100000;
}  // namespace

namespace taxi
{
bool ApiBase::IsDistanceSupported(ms::LatLon const & from, ms::LatLon const & to) const
{
  return ms::DistanceOnEarth(from, to) <= kMaxSupportedDistance;
}

bool ApiItem::AreAllCountriesDisabled(storage::TCountriesVec const & countryIds,
                                      std::string const & city) const
{
  if (countryIds.empty())
    return true;

  auto const & disabled = m_places.m_disabledPlaces;

  if (disabled.IsCountriesEmpty())
    return false;

  bool isCountryDisabled = true;
  for (auto const & countryId : countryIds)
    isCountryDisabled = isCountryDisabled && disabled.Has(countryId, city);

  return isCountryDisabled;
}

bool ApiItem::IsAnyCountryEnabled(storage::TCountriesVec const & countryIds,
                                  std::string const & city) const
{
  if (countryIds.empty())
    return false;

  auto const & enabled = m_places.m_enabledPlaces;

  if (enabled.IsCountriesEmpty())
    return false;

  for (auto const & countryId : countryIds)
  {
    if (enabled.Has(countryId, city))
      return true;
  }

  return false;
}

bool ApiItem::IsMwmDisabled(storage::TCountryId const & mwmId) const
{
  auto const & disabled = m_places.m_disabledPlaces;
  if (mwmId.empty())
    return false;

  return disabled.Has(mwmId);
}

bool ApiItem::IsMwmEnabled(storage::TCountryId const & mwmId) const
{
  auto const & enabled = m_places.m_enabledPlaces;
  if (mwmId.empty())
    return false;

  return enabled.Has(mwmId);
}
}  // namespace taxi

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

  if (m_disabledPlaces.IsCountriesEmpty())
    return false;

  bool isCountryDisabled = true;
  for (auto const & countryId : countryIds)
    isCountryDisabled = isCountryDisabled && m_disabledPlaces.Has(countryId, city);

  return isCountryDisabled;
}

bool ApiItem::IsAnyCountryEnabled(storage::TCountriesVec const & countryIds,
                                  std::string const & city) const
{
  if (countryIds.empty())
    return false;

  if (m_enabledPlaces.IsCountriesEmpty())
    return true;

  for (auto const & countryId : countryIds)
  {
    if (m_enabledPlaces.Has(countryId, city))
      return true;
  }

  return false;
}

bool ApiItem::IsMwmDisabled(storage::TCountryId const & mwmId) const
{
  if (mwmId.empty())
    return false;

  if (m_disabledPlaces.IsMwmsEmpty())
    return false;

  return m_disabledPlaces.Has(mwmId);
}

bool ApiItem::IsMwmEnabled(storage::TCountryId const & mwmId) const
{
  if (mwmId.empty())
    return false;

  if (m_enabledPlaces.IsMwmsEmpty())
    return true;

  return m_enabledPlaces.Has(mwmId);
}
}  // namespace taxi

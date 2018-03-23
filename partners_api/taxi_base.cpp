#include "partners_api/taxi_base.hpp"

#include "geometry/distance_on_sphere.hpp"

namespace
{
// The maximum supported distance in meters by default.
double const kMaxSupportedDistance = 100000;

bool HasMwmId(taxi::Places const & places, storage::TCountryId const & mwmId)
{
  if (mwmId.empty())
    return false;

  if (places.IsMwmsEmpty())
    return true;

  return places.Has(mwmId);
}
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
  return HasMwmId(m_disabledPlaces, mwmId);
}

bool ApiItem::IsMwmEnabled(storage::TCountryId const & mwmId) const
{
  return HasMwmId(m_enabledPlaces, mwmId);
}
}  // namespace taxi

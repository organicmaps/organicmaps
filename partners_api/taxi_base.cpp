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

  if (m_disabledCountries.IsEmpty())
    return false;

  bool isCountryDisabled = true;
  for (auto const & countryId : countryIds)
    isCountryDisabled = isCountryDisabled && m_disabledCountries.Has(countryId, city);

  return isCountryDisabled;
}

bool ApiItem::IsAnyCountryEnabled(storage::TCountriesVec const & countryIds,
                                  std::string const & city) const
{
  if (countryIds.empty())
    return false;

  if (m_enabledCountries.IsEmpty())
    return true;

  for (auto const & countryId : countryIds)
  {
    if (m_enabledCountries.Has(countryId, city))
      return true;
  }

  return false;
}
}  // namespace taxi

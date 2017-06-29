#include "partners_api/taxi_base.hpp"

namespace taxi
{
bool ApiItem::AreAllCountriesDisabled(storage::TCountriesVec const & countryIds,
                                      std::string const & city) const
{
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

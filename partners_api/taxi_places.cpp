#include "partners_api/taxi_places.hpp"

#include <algorithm>

namespace taxi
{
bool Places::IsCountriesEmpty() const { return m_countries.empty(); }
bool Places::IsMwmsEmpty() const { return m_mwmIds.empty(); }

bool Places::Has(storage::TCountryId const & id, std::string const & city) const
{
  auto const countryIt =
      std::find_if(m_countries.cbegin(), m_countries.cend(),
                   [&id](Country const & country) { return country.m_id == id; });

  if (countryIt == m_countries.cend())
    return false;

  auto const & cities = countryIt->m_cities;

  if (cities.empty())
    return true;

  auto const cityIt = std::find_if(cities.cbegin(), cities.cend(),
                                   [&city](std::string const & c) { return c == city; });

  return cityIt != cities.cend();
}

bool Places::Has(storage::TCountryId const & mwmId) const
{
  return m_mwmIds.find(mwmId) != m_mwmIds.cend();
}
}  // namespace taxi

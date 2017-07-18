#include "partners_api/taxi_countries.hpp"

#include <algorithm>

namespace taxi
{
Countries::Countries(std::vector<Country> const & countries) : m_countries(countries) {}

bool Countries::IsEmpty() const { return m_countries.empty(); }

bool Countries::Has(storage::TCountryId const & id, std::string const & city) const
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
}  // namespace taxi

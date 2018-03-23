#pragma once

#include "storage/index.hpp"

#include <string>
#include <unordered_set>
#include <vector>

namespace taxi
{
class Places
{
public:
  struct Country
  {
    storage::TCountryId m_id;
    std::vector<std::string> m_cities;
  };

  Places(std::vector<Country> const & countries,
         std::unordered_set<storage::TCountryId> const & mwmIds = {});

  bool IsCountriesEmpty() const;
  bool IsMwmsEmpty() const;
  bool Has(storage::TCountryId const & id, std::string const & city) const;
  bool Has(storage::TCountryId const & mwmId) const;

private:
  std::vector<Country> m_countries;
  std::unordered_set<storage::TCountryId> m_mwmIds;
};
}  // namespace taxi

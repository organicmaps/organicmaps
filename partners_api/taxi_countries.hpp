#pragma once

#include "storage/index.hpp"

#include <string>
#include <vector>

namespace taxi
{
class Countries
{
public:
  struct Country
  {
    storage::TCountryId m_id;
    std::vector<std::string> m_cities;
  };

  Countries(std::vector<Country> const & countries);

  bool IsEmpty() const;
  bool Has(storage::TCountryId const & id, std::string const & city) const;

private:
  std::vector<Country> m_countries;
};
}  // namespace taxi

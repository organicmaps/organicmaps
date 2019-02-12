#pragma once

#include "storage/storage_defines.hpp"

#include "coding/serdes_json.hpp"

#include "base/visitor.hpp"

#include <string>
#include <unordered_set>
#include <vector>

namespace taxi
{
class Places
{
public:
  friend class PlacesTest;

  struct Country
  {
    storage::CountryId m_id;
    std::vector<std::string> m_cities;

    DECLARE_VISITOR_AND_DEBUG_PRINT(Country, visitor(m_id, "id"), visitor(m_cities, "cities"))
  };

  bool IsCountriesEmpty() const;
  bool IsMwmsEmpty() const;
  bool Has(storage::CountryId const & id, std::string const & city) const;
  bool Has(storage::CountryId const & mwmId) const;

  DECLARE_VISITOR_AND_DEBUG_PRINT(Places, visitor(m_countries, "countries"),
                                  visitor(m_mwmIds, "mwms"))

private:
  std::vector<Country> m_countries;
  std::unordered_set<storage::CountryId> m_mwmIds;
};

struct SupportedPlaces
{
  Places m_enabledPlaces;
  Places m_disabledPlaces;

  DECLARE_VISITOR_AND_DEBUG_PRINT(SupportedPlaces, visitor(m_enabledPlaces, "enabled"),
                                  visitor(m_disabledPlaces, "disabled"))
};
}  // namespace taxi

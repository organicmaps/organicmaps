#include "storage/country_tree_helpers.hpp"

#include "base/logging.hpp"

#include <vector>
#include <utility>

namespace storage
{
CountryId GetTopmostParentFor(CountryTree const & countries, CountryId const & countryId)
{
  std::vector<CountryTree::Node const *> nodes;
  countries.Find(countryId, nodes);
  if (nodes.empty())
  {
    LOG(LWARNING, ("CountryId =", countryId, "not found in m_countries."));
    return {};
  }

  if (nodes.size() > 1)
  {
    // Disputed territory. Has multiple parents.
    return countryId;
  }

  auto result = nodes[0];

  if (!result->HasParent())
    return result->Value().Name();

  auto parent = &(result->Parent());
  while (parent->HasParent())
  {
    result = parent;
    parent = &(result->Parent());
  }

  return result->Value().Name();
}

std::optional<CountryTree> LoadCountriesFromFile(std::string const & path)
{
  Affiliations affiliations;
  CountryNameSynonyms countryNameSynonyms;
  MwmTopCityGeoIds mwmTopCityGeoIds;
  MwmTopCountryGeoIds mwmTopCountryGeoIds;
  CountryTree countries;
  auto const res = LoadCountriesFromFile(path, countries, affiliations, countryNameSynonyms,
                                         mwmTopCityGeoIds, mwmTopCountryGeoIds);

  if (res == -1)
    return {};

  return std::optional<CountryTree>(std::move(countries));
}
}  // namespace storage

#include "storage/country_tree_helpers.hpp"

namespace storage
{
CountryId GetTopmostParentFor(CountryTree const & countries, CountryId const & countryId)
{
  ASSERT(!countryId.empty(), ());

  CountryTree::NodesBufferT nodes;
  countries.Find(countryId, nodes);
  CHECK(!nodes.empty(), (countryId));

  if (nodes.size() > 1)
  {
    // Disputed territory. Has multiple parents.
    CHECK(nodes[0]->HasParent(), ());
    auto const parentId = nodes[0]->Parent().Value().Name();
    for (size_t i = 1; i < nodes.size(); ++i)
      if (nodes[i]->Parent().Value().Name() != parentId)
        return countryId;
    return GetTopmostParentFor(countries, parentId);
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
  CountryTree countries;
  CountriesInfo countriesInfo;
  auto const res = LoadCountriesFromFile(path, countries, countriesInfo);

  if (res == -1)
    return {};

  return std::move(countries);
}
}  // namespace storage

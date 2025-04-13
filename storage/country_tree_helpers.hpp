#pragma once

#include "storage/country_tree.hpp"
#include "storage/storage_defines.hpp"

#include <optional>
#include <string>

namespace storage
{
// Loads CountryTree only, without affiliations/synonyms/catalogIds.
std::optional<CountryTree> LoadCountriesFromFile(std::string const & path);

// Returns topmost country id prior root id or |countryId| itself, if it's already a topmost node or
// disputed territory id if |countryId| is a disputed territory or belongs to disputed territory.
CountryId GetTopmostParentFor(CountryTree const & countries, CountryId const & countryId);
}  // namespace storage

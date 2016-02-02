#pragma once

#include "std/set.hpp"
#include "std/string.hpp"
#include "std/vector.hpp"

namespace storage
{
using TCountryId = string;
using TCountriesSet = set<TCountryId>;
using TCountriesVec = vector<TCountryId>;

extern const storage::TCountryId kInvalidCountryId;

// @TODO(bykoianko) Check in counrtry tree if the countryId valid.
bool IsCountryIdValid(TCountryId const & countryId);
} //  namespace storage

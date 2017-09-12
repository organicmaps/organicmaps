#pragma once

#include "platform/local_country_file.hpp"

#include "std/set.hpp"
#include "std/shared_ptr.hpp"
#include "std/string.hpp"
#include "std/unordered_set.hpp"
#include "std/vector.hpp"

namespace storage
{
using TCountryId = string;
using TCountriesSet = set<TCountryId>;
using TCountriesVec = vector<TCountryId>;

extern const storage::TCountryId kInvalidCountryId;

using TLocalFilePtr = shared_ptr<platform::LocalCountryFile>;

// @TODO(bykoianko) Check in counrtry tree if the countryId valid.
bool IsCountryIdValid(TCountryId const & countryId);
} //  namespace storage

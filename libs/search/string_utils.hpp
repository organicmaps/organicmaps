#pragma once

#include "search/common.hpp"

namespace search
{

QueryString MakeQueryString(std::string s);

// Building searches cannot match apartment identifiers or unindexed postal delivery codes.
// Keep geographical context and leave non-address queries untouched.
std::string RemoveAddressDetails(std::string const & query);

// Use the same split/joined DACH street spellings for geocoder scoring and address validation.
std::vector<std::vector<strings::UniString>> ModifyDACHStreet(std::vector<strings::UniString> const & streetTokens);

}  // namespace search

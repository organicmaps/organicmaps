#pragma once

#include "search/common.hpp"

namespace search
{

QueryString MakeQueryString(std::string s);

// Building searches cannot match apartment identifiers or unindexed postal delivery codes.
// Keep geographical context and leave non-address queries untouched.
std::string RemoveAddressDetails(std::string const & query);

}  // namespace search

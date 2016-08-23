#pragma once

#include "std/string.hpp"
#include "std/vector.hpp"

namespace search
{
// Returns a list of english names of displayed categories for a
// categories search tab. It's guaranteed that the list is the same
// during the application lifetime.
vector<string> const & GetDisplayedCategories();
}  // namespace search

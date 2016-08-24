#pragma once

#include "std/string.hpp"
#include "std/vector.hpp"

namespace search
{
// Returns a list of English names of displayed categories for the
// categories search tab. It's guaranteed that the list remains the
// same during the application lifetime.
vector<string> const & GetDisplayedCategories();
}  // namespace search

#pragma once

#include <initializer_list>

namespace promo
{
using TypesList = std::initializer_list<std::initializer_list<char const *>>;
TypesList const & GetPromoCatalogSightseeingsTypes();
TypesList const & GetPromoCatalogOutdoorTypes();
}  // namespace promo

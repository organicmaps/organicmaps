#include "map/displayed_categories_modifiers.hpp"

#include "base/macros.hpp"

#include <algorithm>
#include <unordered_set>

namespace
{
std::unordered_set<std::string> const kLuggageHeroesSupportedCities{"London", "New York",
                                                                    "Copenhagen"};
}  // namespace

LuggageHeroModifier::LuggageHeroModifier(std::string const & city) : m_city(city) {}

void LuggageHeroModifier::Modify(search::DisplayedCategories::Keys & keys)
{
  static int const kPos = 4;
  static std::string const kCategoryName = "luggagehero";

  auto const supported =
      kLuggageHeroesSupportedCities.find(m_city) != kLuggageHeroesSupportedCities.cend();
  auto const contains = std::find(keys.cbegin(), keys.cend(), kCategoryName) != keys.cend();

  ASSERT_LESS(kPos, keys.size(), ());

  if (supported && !contains)
    keys.insert(keys.cbegin() + kPos, kCategoryName);
  else if (!supported && contains)
    keys.erase(std::remove(keys.begin(), keys.end(), kCategoryName), keys.end());
}

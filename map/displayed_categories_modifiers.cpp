#include "map/displayed_categories_modifiers.hpp"

#include "base/macros.hpp"

#include <algorithm>
#include <vector>

namespace
{
std::vector<std::string> const kSponsoredCategories = {};

search::DisplayedCategories::Keys::const_iterator FindInsertionPlace(
    search::DisplayedCategories::Keys & keys, uint32_t position)
{
  for (auto it = keys.cbegin(); it != keys.cend(); ++it)
  {
    if (position == 0)
      return it;

    // Do not count sponsored categories.
    if (std::find(kSponsoredCategories.cbegin(), kSponsoredCategories.cend(), *it) ==
        kSponsoredCategories.cend())
    {
      position--;
    }
  }
  return keys.cend();
}
}  // namespace

SponsoredCategoryModifier::SponsoredCategoryModifier(std::string const & currentCity,
                                                     SupportedCities const & supportedCities,
                                                     std::string const & categoryName,
                                                     uint32_t position)
  : m_currentCity(currentCity)
  , m_supportedCities(supportedCities)
  , m_categoryName(categoryName)
  , m_position(position)
{}

void SponsoredCategoryModifier::Modify(search::DisplayedCategories::Keys & keys)
{
  auto const supported = m_supportedCities.find(m_currentCity) != m_supportedCities.cend();
  auto const contains = std::find(keys.cbegin(), keys.cend(), m_categoryName) != keys.cend();

  ASSERT_LESS(m_position, keys.size(), ());

  if (supported && !contains)
    keys.insert(FindInsertionPlace(keys, m_position), m_categoryName);
  else if (!supported && contains)
    keys.erase(std::remove(keys.begin(), keys.end(), m_categoryName), keys.end());
}

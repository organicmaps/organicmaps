#include "map/displayed_categories_modifiers.hpp"

#include "partners_api/cian_api.hpp"

#include "base/macros.hpp"

#include <algorithm>

CianModifier::CianModifier(std::string const & city) : m_city(city) {}

void CianModifier::Modify(search::DisplayedCategories::Keys & keys)
{
  static int const kPos = 4;
  static std::string const kCategoryName = "cian";

  auto const supported = cian::Api::IsCitySupported(m_city);
  auto const contains = std::find(keys.cbegin(), keys.cend(), kCategoryName) != keys.cend();

  ASSERT_LESS(kPos, keys.size(), ());

  if (supported && !contains)
    keys.insert(keys.cbegin() + kPos, kCategoryName);
  else if (!supported && contains)
    keys.erase(std::remove(keys.begin(), keys.end(), kCategoryName), keys.end());
}

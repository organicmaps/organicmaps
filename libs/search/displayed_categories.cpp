#include "search/displayed_categories.hpp"

#include "base/macros.hpp"

#include <algorithm>

namespace search
{
DisplayedCategories::DisplayedCategories(CategoriesHolder const & holder) : m_holder(holder)
{
  m_keys = {"category_eat",      "category_hotel",      "category_food",          "category_tourism",
            "category_wifi",     "category_transport",  "category_fuel",          "category_parking",
            "category_shopping", "category_secondhand", "category_atm",           "category_nightlife",
            "category_children", "category_bank",       "category_entertainment", "category_water",
            "category_hospital", "category_pharmacy",   "category_recycling",     "category_rv",
            "category_police",   "category_toilet",     "category_post"};
}

void DisplayedCategories::Modify(CategoriesModifier & modifier)
{
  modifier.Modify(m_keys);
}

std::vector<std::string> const & DisplayedCategories::GetKeys() const
{
  return m_keys;
}
}  // namespace search

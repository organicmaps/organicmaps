#include "search/displayed_categories.hpp"

#include "base/macros.hpp"

#include <algorithm>

namespace search
{
DisplayedCategories::DisplayedCategories(CategoriesHolder const & holder) : m_holder(holder)
{
  m_keys = {"eat",
            "hotel",
            "food",
            "tourism",
            "wifi",
            "transport",
            "fuel",
            "parking",
            "shopping",
            "atm",
            "nightlife",
            "children",
            "bank",
            "entertainment",
            "hospital",
            "pharmacy",
            "police",
            "toilet",
            "post"};
}

void DisplayedCategories::Modify(CategoriesModifier & modifier)
{
  modifier.Modify(m_keys);
}

std::vector<std::string> const & DisplayedCategories::GetKeys() const { return m_keys; }
}  // namespace search

#include "search/displayed_categories.hpp"

#include "base/macros.hpp"

#include <algorithm>

namespace search
{
DisplayedCategories::DisplayedCategories(CategoriesHolder const & holder) : m_holder(holder)
{
  m_keys = {"food", "hotel", "tourism",       "wifi",     "transport", "fuel",   "parking", "shop",
            "atm",  "bank",  "entertainment", "hospital", "pharmacy",  "police", "toilet",  "post"};
}

std::vector<std::string> const & DisplayedCategories::GetKeys() const { return m_keys; }

void DisplayedCategories::InsertKey(std::string const & key, size_t pos)
{
  CHECK_LESS(pos, m_keys.size(), ());
  m_keys.insert(m_keys.cbegin() + pos, key);
}

void DisplayedCategories::RemoveKey(std::string const & key)
{
  m_keys.erase(std::remove(m_keys.begin(), m_keys.end(), key), m_keys.end());
}

bool DisplayedCategories::Contains(std::string const & key) const
{
  return std::find(m_keys.cbegin(), m_keys.cend(), key) != m_keys.cend();
}
}  // namespace search

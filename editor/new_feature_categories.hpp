#pragma once

#include "std/cstdint.hpp"
#include "std/string.hpp"
#include "std/utility.hpp"
#include "std/vector.hpp"

namespace osm
{
/// Category is an UI synonym to our internal "classificator type".
struct Category
{
  Category(uint32_t type, string const & name) : m_type(type), m_name(name) {}
  /// Feature type from classificator.
  uint32_t m_type;
  /// Localized category name. English is used by default.
  string m_name;
};

struct NewFeatureCategories
{
  vector<Category> m_lastUsed;
  vector<Category> m_allSorted;
};
}  // namespace osm

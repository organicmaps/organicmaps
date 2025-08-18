#include "indexer/cuisines.hpp"

#include "indexer/classificator.hpp"

#include "platform/localization.hpp"

#include "base/stl_helpers.hpp"
#include "base/string_utils.hpp"

#include <algorithm>

namespace osm
{
using namespace std;

Cuisines::Cuisines()
{
  auto const & c = classif();

  /// @todo Better to have GetObjectByPath().
  // Assume that "cuisine" hierarchy is one-level.
  uint32_t const cuisineType = c.GetTypeByPath({"cuisine"});
  c.GetObject(cuisineType)
      ->ForEachObject([this](ClassifObject const & o)
  {
    auto const & name = o.GetName();
    m_allCuisines.emplace_back(name, platform::GetLocalizedTypeName("cuisine-" + name));
  });

  sort(m_allCuisines.begin(), m_allCuisines.end(),
       [](auto const & lhs, auto const & rhs) { return lhs.second < rhs.second; });
}

// static
Cuisines const & Cuisines::Instance()
{
  static Cuisines instance;
  return instance;
}

string const & Cuisines::Translate(string const & singleCuisine) const
{
  static string const kEmptyString;
  auto const it = find_if(m_allCuisines.begin(), m_allCuisines.end(),
                          [&](auto const & cuisine) { return cuisine.first == singleCuisine; });
  if (it != m_allCuisines.end())
    return it->second;
  return kEmptyString;
}

AllCuisines const & Cuisines::AllSupportedCuisines() const
{
  return m_allCuisines;
}
}  // namespace osm

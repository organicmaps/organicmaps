#include "indexer/cuisines.hpp"

#include "indexer/classificator.hpp"

#include "platform/localization.hpp"

#include "base/stl_helpers.hpp"
#include "base/string_utils.hpp"

#include <algorithm>

using namespace std;

namespace osm
{
Cuisines::Cuisines()
{
  auto const add = [&](auto const *, uint32_t type) {
    auto const cuisine = classif().GetFullObjectNamePath(type);
    CHECK_EQUAL(cuisine.size(), 2, (cuisine));
    m_allCuisines.emplace_back(
        cuisine[1], platform::GetLocalizedTypeName(classif().GetReadableObjectName(type)));
  };

  auto const cuisineType = classif().GetTypeByPath({"cuisine"});
  classif().GetObject(cuisineType)->ForEachObjectInTree(add, cuisineType);
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
  static const string kEmptyString;
  auto const it = find_if(m_allCuisines.begin(), m_allCuisines.end(),
                          [&](auto const & cuisine) { return cuisine.first == singleCuisine; });
  if (it != m_allCuisines.end())
    return it->second;
  return kEmptyString;
}

AllCuisines const & Cuisines::AllSupportedCuisines() const { return m_allCuisines; }
}  // namespace osm

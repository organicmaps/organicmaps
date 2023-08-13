#include "search/types_skipper.hpp"
#include "search/model.hpp"

#include "indexer/classificator.hpp"
#include "indexer/ftypes_matcher.hpp"

#include "base/stl_helpers.hpp"

#include <algorithm>

using base::StringIL;

namespace search
{
TypesSkipper::TypesSkipper()
{
  Classificator const & c = classif();

  StringIL const arrSkipEmptyName1[] = {
    {"building"}, {"highway"}, {"landuse"}, {"natural"}, {"office"}, {"waterway"}, {"area:highway"}
  };
  for (auto const & e : arrSkipEmptyName1)
    m_skipIfEmptyName[0].push_back(c.GetTypeByPath(e));

  StringIL const arrSkipEmptyName2[] = {
    {"man_made", "chimney"},

    /// @todo Skip all nameless places?
    {"place", "country"},
    {"place", "state"},
    {"place", "county"},
    {"place", "region"},
    {"place", "city"},
    {"place", "town"},
    {"place", "suburb"},
    {"place", "neighbourhood"},
    {"place", "square"}
  };
  for (auto const & e : arrSkipEmptyName2)
    m_skipIfEmptyName[1].push_back(c.GetTypeByPath(e));

  m_skipAlways[0].push_back(c.GetTypeByPath({"isoline"}));

  // Do not index "entrance" only features.
  StringIL const arrSkipSpecialNames1[] = {
    {"entrance"}, {"wheelchair"}
  };
  for (auto const & e : arrSkipSpecialNames1)
    m_skipSpecialNames[0].push_back(c.GetTypeByPath(e));
}

void TypesSkipper::SkipEmptyNameTypes(feature::TypesHolder & types) const
{
  static const TwoLevelPOIChecker dontSkip;

  types.RemoveIf([this](uint32_t type)
  {
    if (dontSkip.IsMatched(type))
      return false;

    ftype::TruncValue(type, 2);
    if (HasType(m_skipIfEmptyName[1], type))
      return true;

    ftype::TruncValue(type, 1);
    if (HasType(m_skipIfEmptyName[0], type))
      return true;

    return false;
  });
}

bool TypesSkipper::SkipAlways(feature::TypesHolder const & types) const
{
  for (auto type : types)
  {
    ftype::TruncValue(type, 1);
    if (HasType(m_skipAlways[0], type))
      return true;
  }
  return false;
}

bool TypesSkipper::SkipSpecialNames(feature::TypesHolder const & types, std::string_view defName) const
{
  // Since we assign ref tag into name for entrances, this is a crutch to avoid indexing
  // these refs (entrance numbers for CIS countries).
  /// @todo Move refs into metadata?

  uint32_t dummy;
  if (!strings::to_uint(defName, dummy))
    return false;

  for (auto type : types)
  {
    if (!HasType(m_skipSpecialNames[0], type))
      return false;
  }
  return true;
}

// static
bool TypesSkipper::HasType(Cont const & v, uint32_t t)
{
  return base::IsExist(v, t);
}
}  // namespace search

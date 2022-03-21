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

  StringIL const typesLengthOne[] = {{"building"}, {"highway"}, {"landuse"}, {"natural"},
                                     {"office"}, {"waterway"}, {"area:highway"}};

  for (auto const & e : typesLengthOne)
    m_skipIfEmptyName[0].push_back(c.GetTypeByPath(e));

  StringIL const typesLengthTwo[] = {{"man_made", "chimney"},
                                     {"place", "country"},
                                     {"place", "state"},
                                     {"place", "county"},
                                     {"place", "region"},
                                     {"place", "city"},
                                     {"place", "town"},
                                     {"place", "suburb"},
                                     {"place", "neighbourhood"},
                                     {"place", "square"}};

  for (auto const & e : typesLengthTwo)
    m_skipIfEmptyName[1].push_back(c.GetTypeByPath(e));

  m_skipAlways[0].push_back(c.GetTypeByPath({"isoline"}));
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
    ftype::TruncValue(type, 2);
    if (HasType(m_skipAlways[1], type))
      return true;

    ftype::TruncValue(type, 1);
    if (HasType(m_skipAlways[0], type))
      return true;
  }
  return false;
}

// static
bool TypesSkipper::HasType(Cont const & v, uint32_t t)
{
  return base::IsExist(v, t);
}
}  // namespace search

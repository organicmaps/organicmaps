#include "types_skipper.hpp"

#include "indexer/classificator.hpp"
#include "indexer/feature_data.hpp"
#include "indexer/ftypes_matcher.hpp"

#include "std/algorithm.hpp"
#include "std/initializer_list.hpp"

namespace search
{
TypesSkipper::TypesSkipper()
{
  Classificator const & c = classif();

  for (auto const & e : (StringIL[]){{"entrance"}})
    m_skipAlways[0].push_back(c.GetTypeByPath(e));
  for (auto const & e : (StringIL[]){{"building", "address"}})
    m_skipAlways[1].push_back(c.GetTypeByPath(e));

  for (auto const & e : (StringIL[]){{"building"}, {"highway"}, {"natural"},
                                     {"waterway"}, {"landuse"}})
  {
    m_skipIfEmptyName[0].push_back(c.GetTypeByPath(e));
  }

  for (auto const & e : (StringIL[]){{"place", "country"},
                                     {"place", "state"},
                                     {"place", "county"},
                                     {"place", "region"},
                                     {"place", "city"},
                                     {"place", "town"},
                                     {"railway", "rail"}})
  {
    m_skipIfEmptyName[1].push_back(c.GetTypeByPath(e));
  }

  m_country = c.GetTypeByPath({"place", "country"});
  m_state = c.GetTypeByPath({"place", "state"});
}

void TypesSkipper::SkipTypes(feature::TypesHolder & types) const
{
  auto shouldBeRemoved = [this](uint32_t type)
  {
    ftype::TruncValue(type, 2);
    if (HasType(m_skipAlways[1], type))
      return true;

    ftype::TruncValue(type, 1);
    if (HasType(m_skipAlways[0], type))
      return true;

    return false;
  };

  types.RemoveIf(shouldBeRemoved);
}

void TypesSkipper::SkipEmptyNameTypes(feature::TypesHolder & types) const
{
  auto shouldBeRemoved = [this](uint32_t type)
  {
    if (m_dontSkipIfEmptyName.Has(type))
      return false;

    ftype::TruncValue(type, 2);
    if (HasType(m_skipIfEmptyName[1], type))
      return true;

    ftype::TruncValue(type, 1);
    if (HasType(m_skipIfEmptyName[0], type))
      return true;

    return false;
  };

  types.RemoveIf(shouldBeRemoved);
}

bool TypesSkipper::IsCountryOrState(feature::TypesHolder const & types) const
{
  for (uint32_t t : types)
  {
    ftype::TruncValue(t, 2);
    if (t == m_country || t == m_state)
      return true;
  }
  return false;
}

// static
bool TypesSkipper::HasType(TCont const & v, uint32_t t)
{
  return find(v.begin(), v.end(), t) != v.end();
}
}  // namespace search

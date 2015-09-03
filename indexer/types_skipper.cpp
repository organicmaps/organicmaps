#include "indexer/types_skipper.hpp"

#include "indexer/classificator.hpp"
#include "indexer/feature_data.hpp"
#include "indexer/ftypes_matcher.hpp"

#include "std/initializer_list.hpp"

namespace search
{
TypesSkipper::TypesSkipper()
{
  Classificator const & c = classif();

  // Fill types that always! should be skipped.
  for (auto const & e : (StringIL[]){{"entrance"}})
    m_skipF[0].push_back(c.GetTypeByPath(e));

  for (auto const & e : (StringIL[]){{"building", "address"}})
    m_skipF[1].push_back(c.GetTypeByPath(e));

  // Fill types that never! will be skipped.
  for (auto const & e : (StringIL[]){{"highway", "bus_stop"}, {"highway", "speed_camera"}})
    m_dontSkipEn.push_back(c.GetTypeByPath(e));

  // Fill types that will be skipped if feature's name is empty!
  for (auto const & e :
       (StringIL[]){{"building"}, {"highway"}, {"natural"}, {"waterway"}, {"landuse"}})
    m_skipEn[0].push_back(c.GetTypeByPath(e));

  for (auto const & e : (StringIL[]){{"place", "country"},
                                     {"place", "state"},
                                     {"place", "county"},
                                     {"place", "region"},
                                     {"place", "city"},
                                     {"place", "town"},
                                     {"railway", "rail"}})
  {
    m_skipEn[1].push_back(c.GetTypeByPath(e));
  }

  m_country = c.GetTypeByPath({"place", "country"});
  m_state = c.GetTypeByPath({"place", "state"});
}

void TypesSkipper::SkipTypes(feature::TypesHolder & types) const
{
  types.RemoveIf([this](uint32_t type)
                 {
                   ftype::TruncValue(type, 2);

                   if (HasType(m_skipF[1], type))
                     return true;

                   ftype::TruncValue(type, 1);

                   if (HasType(m_skipF[0], type))
                     return true;

                   return false;
                 });
}

void TypesSkipper::SkipEmptyNameTypes(feature::TypesHolder & types) const
{
  types.RemoveIf([this](uint32_t type)
                 {
                   ftype::TruncValue(type, 2);

                   if (HasType(m_dontSkipEn, type))
                     return false;

                   if (HasType(m_skipEn[1], type))
                     return true;

                   ftype::TruncValue(type, 1);

                   if (HasType(m_skipEn[0], type))
                     return true;

                   return false;
                 });
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

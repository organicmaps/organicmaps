#include "generator/place.hpp"

#include "indexer/classificator.hpp"
#include "indexer/ftypes_matcher.hpp"

#include "geometry/mercator.hpp"

namespace generator
{
Place::Place(FeatureBuilder1 const & ft, uint32_t type) :
  m_ft(ft),
  m_pt(ft.GetKeyPoint()),
  m_type(type)
{
  using namespace ftypes;

  switch (IsLocalityChecker::Instance().GetType(m_type))
  {
  case COUNTRY: m_thresholdM = 300000.0; break;
  case STATE: m_thresholdM = 100000.0; break;
  case CITY: m_thresholdM = 30000.0; break;
  case TOWN: m_thresholdM = 20000.0; break;
  case VILLAGE: m_thresholdM = 10000.0; break;
  default: m_thresholdM = 10000.0; break;
  }
}

m2::RectD Place::GetLimitRect() const
{
  return MercatorBounds::RectByCenterXYAndSizeInMeters(m_pt, m_thresholdM);
}

bool Place::IsEqual(Place const & r) const
{
  return (AreTypesEqual(m_type, r.m_type) &&
          m_ft.GetName() == r.m_ft.GetName() &&
          (IsPoint() || r.IsPoint()) &&
          MercatorBounds::DistanceOnEarth(m_pt, r.m_pt) < m_thresholdM);
}

bool Place::IsBetterThan(Place const & r) const
{
  // Check ranks.
  uint8_t const r1 = m_ft.GetRank();
  uint8_t const r2 = r.m_ft.GetRank();
  if (r1 != r2)
    return r1 > r2;

  // Check types length.
  // ("place-city-capital-2" is better than "place-city").
  uint8_t const l1 = ftype::GetLevel(m_type);
  uint8_t const l2 = ftype::GetLevel(r.m_type);
  if (l1 != l2)
    return l1 > l2;

  // Assume that area places has better priority than point places at the very end ...
  /// @todo It was usefull when place=XXX type has any area fill style.
  /// Need to review priority logic here (leave the native osm label).
  return !IsPoint() && r.IsPoint();
}

bool Place::AreTypesEqual(uint32_t t1, uint32_t t2)
{
  // Use 2-arity places comparison for filtering.
  // ("place-city-capital-2" is equal to "place-city")
  ftype::TruncValue(t1, 2);
  ftype::TruncValue(t2, 2);
  return (t1 == t2);
}
}  // namespace generator

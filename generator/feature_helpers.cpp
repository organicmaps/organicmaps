#include "generator/feature_helpers.hpp"

#include "generator/feature_builder.hpp"

#include "coding/point_coding.hpp"

#include "base/stl_helpers.hpp"

#include <algorithm>

namespace feature
{
CalculateMidPoints::CalculateMidPoints()
  : CalculateMidPoints(static_cast<int (*)(TypesHolder const & types, m2::RectD limitRect)>(GetMinDrawableScale))
{ }

CalculateMidPoints::CalculateMidPoints(MinDrawableScalePolicy const & minDrawableScalePolicy)
  : m_minDrawableScalePolicy{minDrawableScalePolicy}
{ }

void CalculateMidPoints::operator()(FeatureBuilder const & ft, uint64_t pos)
{
  // Reset state.
  m_midLoc = m2::PointD::Zero();;
  m_locCount = 0;

  ft.ForEachPoint(*this);
  ASSERT_NOT_EQUAL(m_locCount, 0, ());
  m_midLoc = m_midLoc / m_locCount;

  uint64_t const pointAsInt64 = PointToInt64Obsolete(m_midLoc, m_coordBits);
  int const minScale = m_minDrawableScalePolicy(ft.GetTypesHolder(), ft.GetLimitRect());

  /// May be invisible if it's small area object with [0-9] scales.
  /// @todo Probably, we need to keep that objects if 9 scale (as we do in 17 scale).
  if (minScale != -1)
  {
    uint64_t const order = (static_cast<uint64_t>(minScale) << 59) | (pointAsInt64 >> 5);
    m_vec.emplace_back(order, pos);
  }
}

bool CalculateMidPoints::operator()(m2::PointD const & p)
{
  m_midLoc += p;
  m_midAll += p;
  ++m_locCount;
  ++m_allCount;
  return true;
}

m2::PointD CalculateMidPoints::GetCenter() const
{
  if (m_allCount == 0)
    return {};

  return m_midAll / m_allCount;
}

void CalculateMidPoints::Sort()
{
  std::sort(m_vec.begin(), m_vec.end(), base::LessBy(&CellAndOffset::first));
}
}  // namespace feature

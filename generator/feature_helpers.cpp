#include "generator/feature_helpers.hpp"
#include "generator/feature_builder.hpp"

#include "indexer/feature_visibility.hpp"

#include "base/stl_helpers.hpp"

#include <algorithm>

namespace feature
{
CalculateMidPoints::CalculateMidPoints()
{
  m_minDrawableScaleFn = [](FeatureBuilder const & fb)
  { return GetMinDrawableScale(fb.GetTypesHolder(), fb.GetLimitRect()); };
}

void CalculateMidPoints::operator()(FeatureBuilder const & ft, uint64_t pos)
{
  // Reset state.
  m_midLoc = m2::PointD::Zero();
  m_locCount = 0;

  ft.ForEachPoint(*this);
  CHECK_GREATER(m_locCount, 0, ());
  m_midLoc = m_midLoc / m_locCount;

  uint64_t const pointAsInt64 = PointToInt64Obsolete(m_midLoc, m_coordBits);
  int const minScale = m_minDrawableScaleFn(ft);

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
  // Possible in unit tests.
  if (m_allCount == 0)
    return {0, 0};

  return m_midAll / m_allCount;
}

void CalculateMidPoints::Sort()
{
  std::sort(m_vec.begin(), m_vec.end(), base::LessBy(&CellAndOffset::first));
}
}  // namespace feature

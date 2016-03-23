#include "search/v2/nested_rects_cache.hpp"

#include "search/v2/ranking_info.hpp"

#include "indexer/index.hpp"

#include "geometry/mercator.hpp"
#include "geometry/rect2d.hpp"

#include "base/assert.hpp"
#include "base/stl_add.hpp"

#include "std/algorithm.hpp"

namespace search
{
namespace v2
{
namespace
{
double const kPositionToleranceMeters = 15.0;
}  // namespace

NestedRectsCache::NestedRectsCache(Index & index)
  : m_index(index), m_scale(0), m_position(0, 0), m_valid(false)
{
}

void NestedRectsCache::SetPosition(m2::PointD const & position, int scale)
{
  double distance = MercatorBounds::DistanceOnEarth(position, m_position);
  if (distance < kPositionToleranceMeters && scale == m_scale && m_valid)
    return;
  m_position = position;
  m_scale = scale;
  Update();
}

double NestedRectsCache::GetDistanceToFeatureMeters(FeatureID const & id) const
{
  if (!m_valid)
    return RankingInfo::kMaxDistMeters;

  size_t bucket = 0;
  for (; bucket != RECT_SCALE_COUNT; ++bucket)
  {
    if (binary_search(m_features[bucket].begin(), m_features[bucket].end(), id))
      break;
  }
  auto const scale = static_cast<RectScale>(bucket);

  if (scale != RECT_SCALE_COUNT)
    return GetRadiusMeters(scale);

  if (auto const & info = id.m_mwmId.GetInfo())
  {
    auto const & rect = info->m_limitRect;
    return max(MercatorBounds::DistanceOnEarth(rect.Center(), m_position), GetRadiusMeters(scale));
  }

  return RankingInfo::kMaxDistMeters;
}

void NestedRectsCache::Clear()
{
  for (int scale = 0; scale != RECT_SCALE_COUNT; ++scale)
  {
    m_features[scale].clear();
    m_features[scale].shrink_to_fit();
  }
  m_valid = false;
}

// static
double NestedRectsCache::GetRadiusMeters(RectScale scale)
{
  switch (scale)
  {
  case RECT_SCALE_TINY: return 100.0;
  case RECT_SCALE_SMALL: return 300.0;
  case RECT_SCALE_MEDIUM: return 1000.0;
  case RECT_SCALE_LARGE: return 2500.0;
  case RECT_SCALE_COUNT: return 5000.0;
  }
}

void NestedRectsCache::Update()
{
  for (int scale = 0; scale != RECT_SCALE_COUNT; ++scale)
  {
    auto & features = m_features[scale];

    features.clear();
    m2::RectD const rect = MercatorBounds::RectByCenterXYAndSizeInMeters(
        m_position, GetRadiusMeters(static_cast<RectScale>(scale)));
    auto addId = MakeBackInsertFunctor(features);
    m_index.ForEachFeatureIDInRect(addId, rect, m_scale);
    sort(features.begin(), features.end());
  }

  m_valid = true;
}
}  // namespace v2
}  // namespace search

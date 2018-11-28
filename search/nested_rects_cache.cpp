#include "search/nested_rects_cache.hpp"

#include "search/ranking_info.hpp"

#include "indexer/data_source.hpp"

#include "geometry/mercator.hpp"
#include "geometry/rect2d.hpp"

#include "base/assert.hpp"
#include "base/stl_helpers.hpp"

#include "std/algorithm.hpp"

namespace search
{
namespace
{
double const kPositionToleranceMeters = 15.0;
}  // namespace

NestedRectsCache::NestedRectsCache(DataSource const & dataSource)
  : m_dataSource(dataSource), m_scale(0), m_position(0, 0), m_valid(false)
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

  int scale = 0;
  for (; scale != RECT_SCALE_COUNT; ++scale)
  {
    auto const & bucket = m_buckets[scale];
    auto const it = bucket.find(id.m_mwmId);
    if (it == bucket.end())
      continue;
    auto const & features = it->second;
    if (binary_search(features.begin(), features.end(), id.m_index))
      break;
  }

  if (scale != RECT_SCALE_COUNT)
    return GetRadiusMeters(static_cast<RectScale>(scale));

  if (auto const & info = id.m_mwmId.GetInfo())
  {
    auto const & rect = info->m_bordersRect;
    return max(MercatorBounds::DistanceOnEarth(rect.Center(), m_position),
               GetRadiusMeters(static_cast<RectScale>(scale)));
  }

  return RankingInfo::kMaxDistMeters;
}

void NestedRectsCache::Clear()
{
  for (int scale = 0; scale != RECT_SCALE_COUNT; ++scale)
    m_buckets[scale].clear();
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
  UNREACHABLE();
}

void NestedRectsCache::Update()
{
  for (int scale = 0; scale != RECT_SCALE_COUNT; ++scale)
  {
    auto & bucket = m_buckets[scale];
    bucket.clear();

    m2::RectD const rect = MercatorBounds::RectByCenterXYAndSizeInMeters(
        m_position, GetRadiusMeters(static_cast<RectScale>(scale)));

    MwmSet::MwmId lastId;
    TFeatures * lastFeatures = nullptr;
    auto addId = [&lastId, &lastFeatures, &bucket](FeatureID const & id)
    {
      if (!id.IsValid())
        return;
      if (id.m_mwmId != lastId)
      {
        lastId = id.m_mwmId;
        lastFeatures = &bucket[lastId];
      }
      lastFeatures->push_back(id.m_index);
    };
    m_dataSource.ForEachFeatureIDInRect(addId, rect, m_scale);
    for (auto & kv : bucket)
      sort(kv.second.begin(), kv.second.end());
  }

  m_valid = true;
}
}  // namespace search

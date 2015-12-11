#pragma once

#include "indexer/cell_id.hpp"
#include "indexer/drawing_rules.hpp"
#include "indexer/feature_data.hpp"
#include "indexer/feature_visibility.hpp"
#include "indexer/scales.hpp"

#include "geometry/any_rect2d.hpp"
#include "geometry/point2d.hpp"
#include "geometry/screenbase.hpp"
#include "geometry/tree4d.hpp"

#include "std/map.hpp"
#include "std/queue.hpp"
#include "std/vector.hpp"

namespace
{
double constexpr kPOIPerTileSizeCount = 6;
double constexpr kPointLookupDeltaDegrees = 1e-6;
}  //  namespace

namespace covering
{
class CellFeaturePair
{
public:
  CellFeaturePair() = default;
  CellFeaturePair(uint64_t cell, uint32_t feature)
    : m_CellLo(UINT64_LO(cell)), m_CellHi(UINT64_HI(cell)), m_Feature(feature)
  {
  }

  bool operator<(CellFeaturePair const & rhs) const
  {
    if (m_CellHi != rhs.m_CellHi)
      return m_CellHi < rhs.m_CellHi;
    if (m_CellLo != rhs.m_CellLo)
      return m_CellLo < rhs.m_CellLo;
    return m_Feature < rhs.m_Feature;
  }

  uint64_t GetCell() const { return UINT64_FROM_UINT32(m_CellHi, m_CellLo); }
  uint32_t GetFeature() const { return m_Feature; }
private:
  uint32_t m_CellLo;
  uint32_t m_CellHi;
  uint32_t m_Feature;
};
static_assert(sizeof(CellFeaturePair) == 12, "");
#ifndef OMIM_OS_LINUX
static_assert(is_trivially_copyable<CellFeaturePair>::value, "");
#endif

class CellFeatureBucketTuple
{
public:
  CellFeatureBucketTuple() = default;
  CellFeatureBucketTuple(CellFeaturePair const & p, uint32_t bucket) : m_pair(p), m_bucket(bucket)
  {
  }

  bool operator<(CellFeatureBucketTuple const & rhs) const
  {
    if (m_bucket != rhs.m_bucket)
      return m_bucket < rhs.m_bucket;
    return m_pair < rhs.m_pair;
  }

  CellFeaturePair const & GetCellFeaturePair() const { return m_pair; }
  uint32_t GetBucket() const { return m_bucket; }
private:
  CellFeaturePair m_pair;
  uint32_t m_bucket;
};
static_assert(sizeof(CellFeatureBucketTuple) == 16, "");
#ifndef OMIM_OS_LINUX
static_assert(is_trivially_copyable<CellFeatureBucketTuple>::value, "");
#endif

template <class TSorter>
class DisplacementManager
{
public:
  DisplacementManager(TSorter & sorter) : m_sorter(sorter) {}
  template <class TFeature>
  void Add(vector<int64_t> const & cells, uint32_t bucket, TFeature const & ft, uint32_t index)
  {
    if (bucket != scales::GetUpperScale() && IsDisplaceable(ft))
    {
      m_storage[bucket].emplace_back(cells, ft, index, bucket);
      return;
    }

    for (auto const & cell : cells)
      m_sorter.Add(CellFeatureBucketTuple(CellFeaturePair(cell, index), bucket));
  }

  void Displace()
  {
    m4::Tree<DisplaceableNode> acceptedNodes;
    list<DisplaceableNode> deferredNodes;
    for (uint32_t scale = 0; scale < scales::GetUpperScale(); ++scale)
    {
      // Initialize queue.
      priority_queue<DisplaceableNode> displaceableQueue;
      for (auto const & node : m_storage[scale])
        displaceableQueue.push(node);
      for (auto const & node : deferredNodes)
        displaceableQueue.push(node);
      deferredNodes.clear();

      float const delta = CalculateDeltaForZoom(scale);
      size_t accepted = 0;

      // Process queue.
      while (!displaceableQueue.empty())
      {
        DisplaceableNode const maxNode = displaceableQueue.top();
        displaceableQueue.pop();

        // Check if this node was displaced by higher features.
        m2::RectD const displacementRect(maxNode.center, maxNode.center);
        bool isDisplacing = false;
        acceptedNodes.ForEachInRect( m2::Inflate(displacementRect, {delta, delta}),
            [&isDisplacing, &maxNode, &delta, &scale](DisplaceableNode const & node)
            {
              if ((maxNode.center - node.center).Length() < delta && node.maxZoomLevel > scale)
                isDisplacing = true;
            });
        if (isDisplacing)
        {
          deferredNodes.push_back(maxNode);
          continue;
        }

        // Add feature to index otherwise.
        for (auto const & cell : maxNode.cells)
          m_sorter.Add(CellFeatureBucketTuple(CellFeaturePair(cell, maxNode.index), scale));
        acceptedNodes.Add(maxNode);
        accepted++;
      }
      LOG(LINFO, ("Displacement for scale", scale, "Features accepted:", accepted,
                  "Features discarded:", deferredNodes.size()));
    }
  }

private:
  struct DisplaceableNode
  {
    uint32_t index;
    m2::PointD center;
    vector<int64_t> cells;

    int maxZoomLevel;
    uint32_t priority;

    DisplaceableNode() : index(0), maxZoomLevel(0), priority(0) {}
    template <class TFeature>
    DisplaceableNode(vector<int64_t> const & cells, TFeature const & ft, uint32_t index,
                    int zoomLevel)
      : index(index), center(ft.GetCenter()), cells(cells)
    {
      feature::TypesHolder const types(ft);
      auto scaleRange = feature::GetDrawableScaleRange(types);
      maxZoomLevel = scaleRange.second;

      // Calculate depth field
      drule::KeysT keys;
      feature::GetDrawRule(ft, zoomLevel, keys);
      drule::MakeUnique(keys);
      float depth = 0;
      size_t count = keys.size();
      for (size_t i = 0; i < count; ++i)
      {
        if (depth < keys[i].m_priority)
          depth = keys[i].m_priority;
      }

      float const kMinDepth = -100000.0f;
      float const kMaxDepth = 100000.0f;
      float const d = my::clamp(depth, kMinDepth, kMaxDepth) - kMinDepth;
      uint8_t rank = ft.GetRank();
      priority = (static_cast<uint32_t>(d) << 8) | rank;
    }

    bool operator<(DisplaceableNode const & node) const { return priority < node.priority; }
    bool operator==(DisplaceableNode const & node) const { return index == node.index; }
    m2::RectD const GetLimitRect() const { return m2::RectD(center, center); }
  };

  template <class TFeature>
  bool IsDisplaceable(TFeature const & ft) const noexcept
  {
    feature::TypesHolder const types(ft);
    return types.GetGeoType() == feature::GEOM_POINT;
  }

  float CalculateDeltaForZoom(int32_t zoom)
  {
    double const worldSizeDivisor = 1 << zoom;
    // Mercator SizeX and SizeY is equal
    double const rectSize = (MercatorBounds::maxX - MercatorBounds::minX) / worldSizeDivisor;
    return rectSize / kPOIPerTileSizeCount;
  }

  TSorter & m_sorter;
  map<uint32_t, vector<DisplaceableNode>> m_storage;
};
}  // namespace indexer

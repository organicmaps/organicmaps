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
double constexpr kPOIperTileSizeCoint = 6;
double constexpr kPointLookupDeltaDegrees = 0.000001;
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
    if (bucket != scales::GetUpperScale() && IsDisplacable(ft))
    {
      m_storage[bucket].emplace_back(cells, ft, index, bucket);
      return;
    }

    for (auto const & cell : cells)
      m_sorter.Add(CellFeatureBucketTuple(CellFeaturePair(cell, index), bucket));
  }

  void Displace()
  {
    m4::Tree<DisplacableNode> acceptedNodes;
    list<DisplacableNode> deferredNodes;
    for (uint32_t zoom = 0; zoom < scales::GetUpperScale(); ++zoom)
    {
      // Initialize queue.
      priority_queue<DisplacableNode> displacableQueue;
      for (auto const & node : m_storage[zoom])
        displacableQueue.push(node);
      for (auto const & node : deferredNodes)
        displacableQueue.push(node);
      deferredNodes.clear();

      float const delta = CalculateDeltaForZoom(zoom);
      size_t accepted = 0;

      // Process queue.
      while (!displacableQueue.empty())
      {
        ASSERT(!displacableQueue.empty(), ());
        DisplacableNode const maxNode = displacableQueue.top();
        displacableQueue.pop();

        // Check if this node was displaced by higher features.
        m2::RectD const displacementRect(maxNode.center.x - delta, maxNode.center.y - delta,
                                         maxNode.center.x + delta, maxNode.center.y + delta);
        bool isDisplacing = false;
        acceptedNodes.ForEachInRect(
            displacementRect, [&isDisplacing, &maxNode, &delta, &zoom](DisplacableNode const & node)
            {
              if ((maxNode.center - node.center).Length() < delta && node.maxZoomLevel > zoom)
                isDisplacing = true;
            });
        if (isDisplacing)
        {
          deferredNodes.push_back(maxNode);
          continue;
        }

        // Add feature to index otherwise.
        for (auto const & cell : maxNode.cells)
          m_sorter.Add(CellFeatureBucketTuple(CellFeaturePair(cell, maxNode.index), zoom));
        acceptedNodes.Add(maxNode);
        accepted++;
      }
      LOG(LINFO, ("Displacement for zoom", zoom, "Features accepted:", accepted,
                  "Features discarted:", deferredNodes.size()));
    }
  }

private:
  struct DisplacableNode
  {
    uint32_t index;
    m2::PointD center;
    vector<int64_t> cells;

    int maxZoomLevel;
    uint32_t priority;

    DisplacableNode() : index(0), maxZoomLevel(0), priority(0) {}
    template <class TFeature>
    DisplacableNode(vector<int64_t> const & cells, TFeature const & ft, uint32_t index,
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
      float const d = my::clamp(depth, kMinDepth, kMaxDepth) + kMaxDepth;
      uint64_t rank = ft.GetRank();
      priority = (static_cast<uint32_t>(d) << 8) || rank;
    }

    bool operator<(DisplacableNode const & node) const { return priority < node.priority; }
    bool operator==(DisplacableNode const & node) const { return index == node.index; }
    m2::RectD const GetLimitRect() const { return m2::RectD(center, center); }
  };

  template <class TFeature>
  bool IsDisplacable(TFeature const & ft) const noexcept
  {
    feature::TypesHolder const types(ft);
    if (types.GetGeoType() == feature::GEOM_POINT)
      return true;
    return false;
  }

  float CalculateDeltaForZoom(int32_t zoom)
  {
    double const worldSizeDevisor = 1 << zoom;
    // Mercator SizeX and SizeY is equal
    double const rectSize = (MercatorBounds::maxX - MercatorBounds::minX) / worldSizeDevisor;
    return rectSize / kPOIperTileSizeCoint;
  }

  TSorter & m_sorter;
  map<uint32_t, vector<DisplacableNode>> m_storage;
};
}  // namespace indexer

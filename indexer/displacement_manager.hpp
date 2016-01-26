#pragma once

#include "indexer/cell_id.hpp"
#include "indexer/drawing_rules.hpp"
#include "indexer/feature_data.hpp"
#include "indexer/feature_visibility.hpp"
#include "indexer/scales.hpp"

#include "geometry/point2d.hpp"
#include "geometry/rect2d.hpp"
#include "geometry/screenbase.hpp"
#include "geometry/tree4d.hpp"

#include "std/map.hpp"
#include "std/queue.hpp"
#include "std/target_os.hpp"
#include "std/vector.hpp"

namespace
{
double constexpr kPOIPerTileSizeCount = 6;
}  // namespace

namespace covering
{
class CellFeaturePair
{
public:
  CellFeaturePair() : m_cellLo(0), m_cellHi(0), m_feature(0) {}

  CellFeaturePair(uint64_t cell, uint32_t feature)
    : m_cellLo(UINT64_LO(cell)), m_cellHi(UINT64_HI(cell)), m_feature(feature)
  {
  }

  bool operator<(CellFeaturePair const & rhs) const
  {
    if (m_cellHi != rhs.m_cellHi)
      return m_cellHi < rhs.m_cellHi;
    if (m_cellLo != rhs.m_cellLo)
      return m_cellLo < rhs.m_cellLo;
    return m_feature < rhs.m_feature;
  }

  uint64_t GetCell() const { return UINT64_FROM_UINT32(m_cellHi, m_cellLo); }
  uint32_t GetFeature() const { return m_feature; }
private:
  uint32_t m_cellLo;
  uint32_t m_cellHi;
  uint32_t m_feature;
};
static_assert(sizeof(CellFeaturePair) == 12, "");
#ifndef OMIM_OS_LINUX
static_assert(is_trivially_copyable<CellFeaturePair>::value, "");
#endif

class CellFeatureBucketTuple
{
public:
  CellFeatureBucketTuple() : m_bucket(0) {}
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

/// Displacement manager filters incoming single-point features to simplify runtime
/// feature visibility displacement.
template <class TSorter>
class DisplacementManager
{
public:
  DisplacementManager(TSorter & sorter) : m_sorter(sorter) {}

  /// Add feature at bucket (zoom) to displacable queue if possible. Pass to bucket otherwise.
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

  /// Check features intersection and pass result to sorter.
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

        // Check if this node is displaced by higher features.
        m2::RectD const displacementRect(maxNode.center, maxNode.center);
        bool isDisplaced = false;
        acceptedNodes.ForEachInRect(m2::Inflate(displacementRect, {delta, delta}),
            [&isDisplaced, &maxNode, &delta, &scale](DisplaceableNode const & node)
            {
              if (maxNode.center.SquareLength(node.center) < delta * delta && node.maxScale > scale)
                isDisplaced = true;
            });
        if (isDisplaced)
        {
          deferredNodes.push_back(maxNode);
          continue;
        }

        // Add feature to index otherwise.
        AddNodeToSorter(maxNode, scale);
        acceptedNodes.Add(maxNode);
        accepted++;
      }

      LOG(LINFO, ("Displacement for scale", scale, "Features accepted:", accepted,
                  "Features discarded:", deferredNodes.size()));
    }

    // Add all fully displaced features to the bottom scale.
    for (auto const & node : deferredNodes)
      AddNodeToSorter(node, scales::GetUpperScale());
  }

private:
  struct DisplaceableNode
  {
    uint32_t index;
    m2::PointD center;
    vector<int64_t> cells;

    int maxScale;
    uint32_t priority;

    DisplaceableNode() : index(0), maxScale(0), priority(0) {}

    template <class TFeature>
    DisplaceableNode(vector<int64_t> const & cells, TFeature const & ft, uint32_t index,
                    int zoomLevel)
      : index(index), center(ft.GetCenter()), cells(cells)
    {
      feature::TypesHolder const types(ft);
      auto scaleRange = feature::GetDrawableScaleRange(types);
      maxScale = scaleRange.second;

      // Calculate depth field
      drule::KeysT keys;
      feature::GetDrawRule(ft, zoomLevel, keys);
      drule::MakeUnique(keys);
      float depth = 0;
      for (size_t i = 0, count = keys.size(); i < count; ++i)
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
    m2::RectD const GetLimitRect() const { return m2::RectD(center, center); }
  };

  template <class TFeature>
  bool IsDisplaceable(TFeature const & ft) const
  {
    feature::TypesHolder const types(ft);
    return types.GetGeoType() == feature::GEOM_POINT;
  }

  float CalculateDeltaForZoom(int32_t zoom) const
  {
    double const worldSizeDivisor = 1 << zoom;
    // Mercator SizeX and SizeY is equal
    double const rectSize = (MercatorBounds::maxX - MercatorBounds::minX) / worldSizeDivisor;
    return rectSize / kPOIPerTileSizeCount;
  }

  void AddNodeToSorter(DisplaceableNode const & node, uint32_t scale)
  {
    for (auto const & cell : node.cells)
      m_sorter.Add(CellFeatureBucketTuple(CellFeaturePair(cell, node.index), scale));
  }

  TSorter & m_sorter;
  map<uint32_t, vector<DisplaceableNode>> m_storage;
};
}  // namespace indexer

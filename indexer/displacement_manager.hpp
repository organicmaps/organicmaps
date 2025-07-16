#pragma once

#include "indexer/cell_id.hpp"
#include "indexer/cell_value_pair.hpp"
#include "indexer/classificator.hpp"
#include "indexer/drawing_rule_def.hpp"
#include "indexer/feature_data.hpp"
#include "indexer/feature_visibility.hpp"
#include "indexer/scales.hpp"

#include "geometry/point2d.hpp"
#include "geometry/rect2d.hpp"
#include "geometry/screenbase.hpp"
#include "geometry/tree4d.hpp"

#include "std/target_os.hpp"

#include <algorithm>
#include <functional>
#include <vector>

namespace covering
{
double constexpr kPOIDisplacementRadiusPixels = 80.;

// Displacement radius in pixels * half of the world in degrees / meaned graphics tile size.
// So average displacement radius will be: this / tiles in row count.
double constexpr kPOIDisplacementRadiusMultiplier = kPOIDisplacementRadiusPixels * 180. / 512.;

class CellFeatureBucketTuple
{
public:
  using CellFeaturePair = CellValuePair<uint32_t>;

  CellFeatureBucketTuple() : m_bucket(0) {}
  CellFeatureBucketTuple(CellFeaturePair const & p, uint32_t bucket) : m_pair(p), m_bucket(bucket) {}

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

static_assert(sizeof(CellFeatureBucketTuple) == 16);
static_assert(std::is_trivially_copyable<CellFeatureBucketTuple>::value);

/// Displacement manager filters incoming single-point features to simplify runtime
/// feature visibility displacement.
template <typename Sorter>
class DisplacementManager
{
public:
  using CellFeaturePair = CellFeatureBucketTuple::CellFeaturePair;

  explicit DisplacementManager(Sorter & sorter) : m_sorter(sorter) {}

  /// Add feature at bucket (zoom) to displaceable queue if possible. Pass to bucket otherwise.
  template <typename Feature>
  void Add(std::vector<int64_t> const & cells, uint32_t bucket, Feature & ft, uint32_t index)
  {
    // Add to displaceable storage if we need to displace POI.
    if (bucket != scales::GetUpperScale() && IsDisplaceable(ft))
    {
      m_storage.emplace_back(cells, ft, index, bucket);
      return;
    }

    // Pass feature to the index otherwise.
    for (auto const & cell : cells)
      m_sorter(CellFeatureBucketTuple(CellFeaturePair(cell, index), bucket));
  }

  /// Check features intersection and supress drawing of intersected features.
  /// As a result some features may have bigger scale parameter than style describes.
  /// But every feature has MaxScale at least.
  /// After all features passed to sorter.
  void Displace()
  {
    m4::Tree<DisplaceableNode> acceptedNodes;

    // Sort in priority descend mode.
    std::sort(m_storage.begin(), m_storage.end(), std::greater<DisplaceableNode>());

    for (auto const & node : m_storage)
    {
      auto scale = node.m_minScale;
      // Do not filter high level objects. Including metro and country names.
      static auto const maximumIgnoredZoom =
          feature::GetDrawableScaleRange(classif().GetTypeByPath({"railway", "station", "subway"})).first;

      if (maximumIgnoredZoom < 0 || scale <= maximumIgnoredZoom)
      {
        AddNodeToSorter(node, static_cast<uint32_t>(scale));
        acceptedNodes.Add(node);
        continue;
      }

      for (; scale < scales::GetUpperScale(); ++scale)
      {
        float const delta = CalculateDeltaForZoom(scale);
        float const squaredDelta = delta * delta;

        m2::RectD const displacementRect(node.m_center, node.m_center);
        bool isDisplaced = false;
        acceptedNodes.ForEachInRect(m2::Inflate(displacementRect, {delta, delta}),
                                    [&isDisplaced, &node, &squaredDelta, &scale](DisplaceableNode const & rhs)
        {
          if (node.m_center.SquaredLength(rhs.m_center) < squaredDelta && rhs.m_maxScale > scale)
            isDisplaced = true;
        });
        if (isDisplaced)
          continue;

        // Add feature to index otherwise.
        AddNodeToSorter(node, scale);
        acceptedNodes.Add(node);
        break;
      }

      if (scale == scales::GetUpperScale())
        AddNodeToSorter(node, scale);
    }
  }

private:
  struct DisplaceableNode
  {
    uint32_t m_index;
    FeatureID m_fID;
    m2::PointD m_center;
    std::vector<int64_t> m_cells;

    int m_minScale;
    int m_maxScale;
    uint32_t m_priority;

    DisplaceableNode() : m_index(0), m_minScale(0), m_maxScale(0), m_priority(0) {}

    template <typename Feature>
    DisplaceableNode(std::vector<int64_t> const & cells, Feature & ft, uint32_t index, int zoomLevel)
      : m_index(index)
      , m_fID(ft.GetID())
      , m_center(ft.GetCenter())
      , m_cells(cells)
      , m_minScale(zoomLevel)
    {
      feature::TypesHolder const types(ft);
      m_maxScale = feature::GetDrawableScaleRange(types).second;

      // Calculate depth field
      drule::KeysT keys;
      feature::GetDrawRule(types, zoomLevel, keys);

      // While the function has "runtime" in its name, it merely filters by metadata-based rules.
      feature::FilterRulesByRuntimeSelector(ft, zoomLevel, keys);
      drule::MakeUnique(keys);

      float depth = 0;
      for (auto const & k : keys)
        if (depth < k.m_priority)
          depth = k.m_priority;

      // @todo: make sure features are prioritised the same way as in the run-time displacer,
      // see overlay_handle.cpp::CalculateOverlayPriority()
      ASSERT(-drule::kOverlaysMaxPriority <= depth && depth < drule::kOverlaysMaxPriority, (depth));
      uint8_t rank = ft.GetRank();
      m_priority = (static_cast<uint32_t>(depth) << 8) | rank;
    }

    // Same to dynamic displacement behaviour.
    bool operator>(DisplaceableNode const & rhs) const
    {
      if (m_priority > rhs.m_priority)
        return true;
      return (m_priority == rhs.m_priority && m_fID < rhs.m_fID);
    }
    m2::RectD const GetLimitRect() const { return m2::RectD(m_center, m_center); }
  };

  template <typename Feature>
  static bool IsDisplaceable(Feature & ft)
  {
    feature::TypesHolder const types(ft);
    return types.GetGeomType() == feature::GeomType::Point;
  }

  float CalculateDeltaForZoom(int32_t zoom) const
  {
    // zoom - 1 is similar to drape.
    double const worldSizeDivisor = 1 << (zoom - 1);
    return kPOIDisplacementRadiusMultiplier / worldSizeDivisor;
  }

  void AddNodeToSorter(DisplaceableNode const & node, uint32_t scale)
  {
    for (auto const & cell : node.m_cells)
      m_sorter(CellFeatureBucketTuple(CellFeaturePair(cell, node.m_index), scale));
  }

  Sorter & m_sorter;
  std::vector<DisplaceableNode> m_storage;
};
}  // namespace covering

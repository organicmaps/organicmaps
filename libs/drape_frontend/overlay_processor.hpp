#pragma once

#include "drape_frontend/shape_view_params.hpp"
#include "drape_frontend/tile_key.hpp"

#include "indexer/road_shields_parser.hpp"

#include "geometry/rect2d.hpp"
#include "geometry/spline.hpp"

#include <string>
#include <unordered_map>
#include <vector>

class ShieldRuleProto;

#define DEBUG_OVERLAY_PROCESSOR

namespace df
{
/// Runtime line feature merger and path text placer.
/// Collects individual line features, merges connected ones by name,
/// orients geometry left-to-right, and produces clipped splines for PathTextShape creation.
/// Street names above geometry, PT route labels below, shields in between.
class OverlayProcessor
{
public:
  OverlayProcessor(m2::RectD const & tileRect, TileKey const & tileKey, double scaleGtoP);

  struct ShieldInfo
  {
    ShieldRuleProto const * m_shieldRule;
    ftypes::RoadShieldsSetT m_roadShields;
    float m_depth;

    ShieldInfo() : m_shieldRule(nullptr), m_depth(0) {}
    bool HasShield() const { return m_shieldRule != nullptr; }
  };

  /// Collect a line feature for street name text and shields.
  void CollectFeature(std::string const & key, m2::SharedSpline const & spline,
                      PathTextViewParams const & params, ShieldInfo const & shield, bool hasPT);

  /// Collect a line feature for PT route text (separate merge by PT text).
  void CollectPTFeature(m2::SharedSpline const & spline, PathTextViewParams const & params);

  /// Used in unit tests.
  /// Result depends on the seed (first element in splines).
  static std::vector<std::vector<m2::PointD>> MergeSplines(std::vector<m2::SharedSpline> const & splines);

  struct MergedChain
  {
    std::vector<m2::SharedSpline> m_clippedSplines;
    PathTextViewParams m_params;
    ShieldInfo m_shield;
    bool m_hasPT = false;  ///< True if any source feature also has PT route text.
#ifdef DEBUG_OVERLAY_PROCESSOR
    std::vector<m2::PointD> m_junctionPoints;
#endif
  };

  std::vector<MergedChain> BuildChains();
  std::vector<MergedChain> BuildPTChains();

  TileKey const & GetTileKey() const { return m_tileKey; }
  double GetScaleGtoP() const { return m_scaleGtoP; }

private:
  struct FeatureData
  {
    m2::SharedSpline m_spline;
    PathTextViewParams m_params;
  };

  struct StreetFeatureData : public FeatureData
  {
    ShieldInfo m_shield;
    bool m_hasPT = false;
  };

  m2::RectD m_tileRect;
  TileKey m_tileKey;
  double m_scaleGtoP;

  /// Street name features grouped by main text.
  std::unordered_map<std::string, std::vector<StreetFeatureData>> m_featuresByName;
  /// PT route features grouped by PT text.
  std::unordered_map<std::string, std::vector<FeatureData>> m_ptFeaturesByText;
};
}  // namespace df

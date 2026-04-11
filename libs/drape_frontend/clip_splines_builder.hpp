#pragma once

#include "geometry/point2d.hpp"
#include "geometry/spline.hpp"

#include <vector>

class FeatureType;

namespace df
{
struct ApplyFeatureParams;

/// Builds a (simplified) line geometry and produces tile-clipped splines from
/// it without the redundant direction/length computations that an intermediate
/// m2::Spline would otherwise perform: directions and lengths are built only
/// on the final clipped pieces that rendering actually consumes.
class ClipSplinesBuilder
{
public:
  explicit ClipSplinesBuilder(ApplyFeatureParams const & params) : m_params(params) {}

  /// Reads the feature path with optional simplification (driven by params).
  /// Uses f.GetLimitRect() to short-circuit Outside features (path stays empty)
  /// and to remember Inside features for a faster Release() path.
  /// When |isIsoline| is true the limit rect is checked against an inflated
  /// tile rect (matching the post-smoothing buffer used by Release()), so
  /// isolines whose tight rect just misses the tile but whose control points
  /// reach into the smoothing buffer aren't dropped. The Inside hint is
  /// suppressed in this mode since Release() ignores it on the isoline path.
  void Build(FeatureType & f, int zoomLevel, bool isIsoline = false);

  bool HasGeometry() const { return m_path.size() > 1; }
  size_t GetPathSize() const { return m_path.size(); }
  std::vector<m2::PointD> const & GetPath() const { return m_path; }
  bool IsKnownInside() const { return m_knownInside; }

  /// Clips the built path against the tile rect (isoline pipeline if isIsoline).
  /// In the Inside case m_path is moved out into the resulting spline; otherwise
  /// no cleanup is performed — the next Build() resets all state.
  std::vector<m2::SharedSpline> Release(bool isIsoline);

private:
  ApplyFeatureParams const & m_params;

  std::vector<m2::PointD> m_path;
  bool m_knownInside = false;
};
}  // namespace df

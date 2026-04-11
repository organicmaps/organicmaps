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
  /// Cached relationship between the feature's limit rect and the tile rect,
  /// determined upfront from f.GetLimitRect() so the regular bbox pass over
  /// the path can be skipped (and Outside features short-circuited entirely).
  enum class CaseHint
  {
    Unknown,  // could be Inside, Outside, or Intersect — let the clip pipeline decide
    Inside,   // feature lies fully inside the tile rect
  };

  explicit ClipSplinesBuilder(ApplyFeatureParams const & params);

  /// Reads the feature path with optional simplification (driven by params).
  /// Uses f.GetLimitRect() to short-circuit Outside features (path stays empty)
  /// and to remember Inside features for a faster Release() path.
  void Build(FeatureType & f, int zoomLevel);

  bool HasGeometry() const { return m_path.size() > 1; }
  size_t GetPathSize() const { return m_path.size(); }
  std::vector<m2::PointD> const & GetPath() const { return m_path; }
  CaseHint GetCaseHint() const { return m_caseHint; }

  /// Clips the built path against the tile rect (isoline pipeline if isIsoline).
  /// In the Inside case m_path is moved out into the resulting spline; otherwise
  /// no cleanup is performed — the next Build() resets all state.
  std::vector<m2::SharedSpline> Release(bool isIsoline);

private:
  ApplyFeatureParams const & m_params;

  std::vector<m2::PointD> m_path;
  CaseHint m_caseHint = CaseHint::Unknown;
};
}  // namespace df

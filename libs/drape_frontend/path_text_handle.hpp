#pragma once

#include "drape_frontend/text_handle.hpp"
#include "drape_frontend/text_layout.hpp"

#include "geometry/spline.hpp"

#include <memory>
#include <vector>

namespace df
{
class PathTextContext
{
public:
  PathTextContext(m2::SharedSpline const & spline, double xOffset = 0.0);

  void SetLayout(drape_ptr<PathTextLayout> && layout, double baseGtoPScale);
  ref_ptr<PathTextLayout> const GetLayout() const;
  double GetVisualScale() const { return m_visualScale; }

  bool GetGlobalPivot(size_t textIndex, m2::PointD & pivot) const;
  bool GetPivot(size_t textIndex, ScreenBase const & screen, m2::PointD & pivot,
                m2::Spline::iterator & centerPointIter);

  void BeforeUpdate();
  void Update(ScreenBase const & screen);

  std::vector<double> const & GetOffsets() const;

private:
  struct ProjectionCursor
  {
    size_t m_splineIndex = 0;
    size_t m_segmentIndex = 0;
    double m_segmentStep = 0.0;
  };

  struct PivotInfo
  {
    m2::Spline::iterator m_centerPointIter;
    bool m_isCalculated = false;
    bool m_isValid = false;
  };

  m2::Spline::iterator GetProjectedPoint(m2::PointD const & pt, ProjectionCursor & cursor) const;

private:
  std::vector<m2::PointD> m_globalPivots;
  std::vector<double> m_globalOffsets;
  m2::SharedSpline m_globalSpline;

  std::vector<m2::SplineEx> m_pixel3dSplines;
  std::vector<PivotInfo> m_pivots;
  ProjectionCursor m_projectionCursor;

  drape_ptr<PathTextLayout> m_layout;
  double const m_visualScale;
  double m_xOffset = 0.0;
  bool m_updated = false;
};

class PathTextHandle : public df::TextHandle
{
public:
  PathTextHandle(dp::OverlayID const & id, std::shared_ptr<PathTextContext> const & context, float depth,
                 uint32_t textIndex, uint64_t priority, ref_ptr<dp::TextureManager> textureManager, int minVisibleScale,
                 bool isBillboard);

  void BeforeUpdate() override;
  bool Update(ScreenBase const & screen) override;

  m2::RectD GetPixelRect(ScreenBase const & screen, bool perspective) const override;
  void GetPixelShape(ScreenBase const & screen, bool perspective, Rects & rects) const override;
  void GetAttributeMutation(ref_ptr<dp::AttributeBufferMutator> mutator) const override;
  bool Enable3dExtention() const override;
  bool HasLinearFeatureShape() const override;

private:
  m2::RectD GetCoarsePixelRect(ScreenBase const & screen, m2::PointD const & pixelPivot, bool perspective) const;
  bool CanSkipPreciseGeometry(ScreenBase const & screen) const;

  m2::PointD m_globalPivot;
  std::shared_ptr<PathTextContext> m_context;
  uint32_t const m_textIndex;
  float const m_depth;

  bool m_hasDynamicGeometry = false;
  mutable bool m_dynamicGeometryDirty = true;
};

/// @name Used in unit-tests only.
/// @{
bool IsValidSplineTurn(m2::PointD const & normalizedDir1, m2::PointD const & normalizedDir2);
void AddPointAndRound(m2::SplineEx & spline, m2::PointD const & pt);
/// @}
}  // namespace df

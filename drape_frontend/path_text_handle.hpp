#pragma once

#include "drape_frontend/text_handle.hpp"
#include "drape_frontend/text_layout.hpp"

#include "geometry/spline.hpp"

#include <algorithm>
#include <memory>
#include <vector>

namespace df
{
class PathTextContext
{
public:
  PathTextContext(m2::SharedSpline const & spline);

  void SetLayout(drape_ptr<PathTextLayout> && layout, double baseGtoPScale);
  ref_ptr<PathTextLayout> const GetLayout() const;

  bool GetPivot(size_t textIndex, m2::PointD & pivot, m2::Spline::iterator & centerPointIter) const;

  void BeforeUpdate();
  void Update(ScreenBase const & screen);

  std::vector<double> const & GetOffsets() const;

private:
  m2::Spline::iterator GetProjectedPoint(m2::PointD const & pt) const;

private:
  std::vector<m2::PointD> m_globalPivots;
  std::vector<double> m_globalOffsets;
  m2::SharedSpline m_globalSpline;

  std::vector<m2::SplineEx> m_pixel3dSplines;
  std::vector<m2::Spline::iterator> m_centerPointIters;
  std::vector<m2::PointD> m_centerGlobalPivots;

  drape_ptr<PathTextLayout> m_layout;
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
  m2::SharedSpline m_spline;
  std::shared_ptr<PathTextContext> m_context;
  uint32_t const m_textIndex;
  m2::PointD m_globalPivot;
  float const m_depth;
};

bool IsValidSplineTurn(m2::PointD const & normalizedDir1, m2::PointD const & normalizedDir2);
void AddPointAndRound(m2::SplineEx & spline, m2::PointD const & pt);
}  // namespace df

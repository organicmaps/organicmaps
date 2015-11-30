#pragma once

#include "drape_frontend/map_shape.hpp"
#include "drape_frontend/shape_view_params.hpp"

#include "geometry/spline.hpp"

namespace df
{

class PathTextShape : public MapShape
{
public:
  PathTextShape(m2::SharedSpline const & spline, PathTextViewParams const & params);
  void Draw(ref_ptr<dp::Batcher> batcher, ref_ptr<dp::TextureManager> textures) const override;
  MapShapePriority GetPriority() const override { return MapShapePriority::LinePriority; }

private:
  uint64_t GetOverlayPriority() const;

  m2::SharedSpline m_spline;
  PathTextViewParams m_params;
};

} // namespace df

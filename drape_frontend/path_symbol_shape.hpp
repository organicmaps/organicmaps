#pragma once

#include "drape_frontend/map_shape.hpp"
#include "drape_frontend/shape_view_params.hpp"

#include "geometry/spline.hpp"

namespace df
{
class PathSymbolShape : public MapShape
{
public:
  PathSymbolShape(m2::SharedSpline const & spline, PathSymbolViewParams const & params);
  void Draw(ref_ptr<dp::GraphicsContext> context, ref_ptr<dp::Batcher> batcher,
            ref_ptr<dp::TextureManager> textures) const override;

private:
  PathSymbolViewParams m_params;
  m2::SharedSpline m_spline;
};
}  // namespace df

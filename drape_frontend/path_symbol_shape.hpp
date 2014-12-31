#pragma once

#include "map_shape.hpp"
#include "shape_view_params.hpp"

#include "../geometry/spline.hpp"

namespace df
{

class PathSymbolShape : public MapShape
{
public:
  PathSymbolShape(m2::SharedSpline const & spline, PathSymbolViewParams const & params);
  virtual void Draw(dp::RefPointer<dp::Batcher> batcher, dp::RefPointer<dp::TextureManager> textures) const;

private:
  PathSymbolViewParams m_params;
  m2::SharedSpline m_spline;
};

}

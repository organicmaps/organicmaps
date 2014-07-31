#pragma once

#include "map_shape.hpp"
#include "shape_view_params.hpp"
#include "common_structures.hpp"

#include "../std/vector.hpp"

#include "../geometry/point2d.hpp"
#include "../geometry/spline.hpp"

namespace df
{

class PathSymbolShape : public MapShape
{
public:
  PathSymbolShape(vector<m2::PointF> const & path, PathSymbolViewParams const & params, float maxScale);
  virtual void Draw(RefPointer<Batcher> batcher, RefPointer<TextureSetHolder> textures) const;

private:
  PathSymbolViewParams m_params;
  m2::Spline m_path;
  float m_maxScale;
};

}

#pragma once

#include "shape_view_params.hpp"
#include "map_shape.hpp"

#include "../drape/batcher.hpp"
#include "../drape/pointers.hpp"
#include "../drape/color.hpp"

#include "../geometry/point2d.hpp"
#include "../std/vector.hpp"

namespace df
{

class AreaShape : public MapShape
{
public:
  AreaShape(vector<m2::PointF> const & triangleList, AreaViewParams const & params);

  virtual void Draw(dp::RefPointer<dp::Batcher> batcher, dp::RefPointer<dp::TextureSetHolder> /*textures*/) const;

private:
  vector<Point3D> m_vertexes;
  AreaViewParams m_params;
};

} // namespace df

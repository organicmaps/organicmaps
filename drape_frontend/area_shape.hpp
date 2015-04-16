#pragma once

#include "drape_frontend/map_shape.hpp"
#include "drape_frontend/shape_view_params.hpp"

#include "drape/pointers.hpp"

#include "geometry/point2d.hpp"
#include "std/vector.hpp"

namespace df
{

class AreaShape : public MapShape
{
public:
  AreaShape(vector<m2::PointF> && triangleList, AreaViewParams const & params);

  virtual void Draw(ref_ptr<dp::Batcher> batcher, ref_ptr<dp::TextureManager> textures) const;

private:
  vector<m2::PointF> m_vertexes;
  AreaViewParams m_params;
};

} // namespace df

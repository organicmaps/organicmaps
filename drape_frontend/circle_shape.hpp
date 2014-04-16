#pragma once

#include "map_shape.hpp"
#include "shape_view_params.hpp"

namespace df
{

class CircleShape : public MapShape
{
public:
  CircleShape(m2::PointF const & mercatorPt, CircleViewParams const & params);

  virtual void Draw(RefPointer<Batcher> batcher, RefPointer<TextureSetHolder> /*textures*/) const;

private:
  m2::PointF m_pt;
  CircleViewParams m_params;
};

} //namespace df

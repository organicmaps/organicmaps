#pragma once

#include "map_shape.hpp"
#include "shape_view_params.hpp"

#include "../geometry/point2d.hpp"

#include "../std/vector.hpp"

namespace df
{

class LineShape : public MapShape
{
public:
  LineShape(vector<m2::PointF> const & points,
            LineViewParams const & params);

  virtual void Draw(RefPointer<Batcher> batcher, RefPointer<TextureSetHolder> /*textures*/) const;

  float         GetWidth() const { return m_params.m_width; }
  Color const & GetColor() const { return m_params.m_color; }

private:
  vector<m2::PointF> m_points;
  LineViewParams m_params;
};

} // namespace df


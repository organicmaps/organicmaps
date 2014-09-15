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

  virtual void Draw(dp::RefPointer<dp::Batcher> batcher, dp::RefPointer<dp::TextureSetHolder> textures) const;

  float         GetWidth() const { return m_params.m_width; }
  dp::Color const & GetColor() const { return m_params.m_color; }

private:
  LineViewParams m_params;
  vector<m2::PointF> m_points;
};

} // namespace df


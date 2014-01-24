#pragma once

#include "map_shape.hpp"

#include "../drape/color.hpp"
#include "../geometry/point2d.hpp"

#include "../std/vector.hpp"

namespace df
{

  class LineShape : public MapShape
  {
  public:
    LineShape(vector<m2::PointF> const & points,
              Color const & color,
              float depth,
              float width);

    virtual void Draw(RefPointer<Batcher> batcher) const;

    float         GetWidth() { return m_width; }
    Color const & GetColor() { return m_color; }

  private:
    vector<m2::PointF> m_points;
    Color m_color;
    float m_depth;
    float m_width;
  };

}


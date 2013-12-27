#pragma once

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
    AreaShape(Color const & c);

    void AddTriangle(const m2::PointF & v1,
                     const m2::PointF & v2,
                     const m2::PointF & v3);

    virtual void Draw(RefPointer<Batcher> batcher) const;

  private:
    Color m_color;
    vector<m2::PointF> m_vertexes;
  };
}

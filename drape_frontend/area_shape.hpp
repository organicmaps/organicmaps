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
    void SetDepth(float depth);

    void AddTriangle(const m2::PointF & v1,
                     const m2::PointF & v2,
                     const m2::PointF & v3);

    virtual void Draw(RefPointer<Batcher> batcher) const;

  private:

    struct Point3D
    {
      float m_x;
      float m_y;
      float m_z;

      Point3D(float x, float y, float z)
        : m_x(x)
        , m_y(y)
        , m_z(z)
      {}
    };

    Color m_color;
    vector<Point3D> m_vertexes;
    float m_depth;
  };
}

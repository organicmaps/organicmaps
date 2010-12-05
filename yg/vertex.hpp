#pragma once

#include "color.hpp"
#include "../geometry/point2d.hpp"

namespace yg
{
  /// Vertex Data Element
  namespace gl
  {
    struct Vertex
    {
      m2::PointF pt;
      float depth;
      m2::PointF tex;

      static const int vertexOffset = 0;
      static const int texCoordOffset = sizeof(m2::PointF) + sizeof(float);

      Vertex();
      Vertex(m2::PointF const & _pt, float _depth = 0, m2::PointF const & _tex = m2::PointF());
      Vertex(Vertex const & v);
      Vertex const & operator=(Vertex const & v);

      static void setupLayout();
    };
  }
}

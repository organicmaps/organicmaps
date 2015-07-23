#pragma once

#include "graphics/color.hpp"
#include "graphics/vertex_decl.hpp"

#include "geometry/point2d.hpp"

namespace graphics
{
  /// Vertex Data Element
  namespace gl
  {
    struct Vertex
    {
      m2::PointF pt;
      float depth;
      m2::PointF normal;
      m2::PointF tex;

      static const int vertexOffset = 0;
      static const int normalOffset = sizeof(m2::PointF) + sizeof(float);
      static const int texCoordOffset = sizeof(m2::PointF) + sizeof(float) + sizeof(m2::PointF);

      Vertex();
      Vertex(m2::PointF const & _pt,
             float _depth = 0,
             m2::PointF const & _normal = m2::PointF(),
             m2::PointF const & _tex = m2::PointF());
      Vertex(Vertex const & v);
      Vertex const & operator=(Vertex const & v);

      static VertexDecl const * getVertexDecl();
    };
  }
}

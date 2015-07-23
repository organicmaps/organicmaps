#pragma once

#include "graphics/color.hpp"
#include "graphics/vertex_decl.hpp"

#include "geometry/point2d.hpp"

namespace graphics
{
  namespace gl
  {
    struct RouteVertex
    {
      m2::PointF pt;
      float depth;
      m2::PointF normal;
      m2::PointF length;
      float lengthZ;

      static const int vertexOffset = 0;
      static const int normalOffset = sizeof(m2::PointF) + sizeof(float);
      static const int lengthOffset = sizeof(m2::PointF) + sizeof(float) + sizeof(m2::PointF);

      RouteVertex();
      RouteVertex(m2::PointF const & _pt, float const _depth, m2::PointF const & _normal,
                  m2::PointF const & _length, float const _lengthZ);
      RouteVertex(RouteVertex const & v);
      RouteVertex const & operator=(RouteVertex const & v);

      static VertexDecl const * getVertexDecl();
    };
  }
}

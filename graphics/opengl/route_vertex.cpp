#include "graphics/opengl/route_vertex.hpp"
#include "graphics/opengl/opengl.hpp"

#include "base/macros.hpp"

namespace graphics
{
namespace gl
{

RouteVertex::RouteVertex()
{}

RouteVertex::RouteVertex(RouteVertex const & v)
  : pt(v.pt),
    depth(v.depth),
    normal(v.normal),
    length(v.length),
    lengthZ(v.lengthZ)
{}

RouteVertex::RouteVertex(m2::PointF const & _pt, float const _depth, m2::PointF const & _normal,
                         m2::PointF const & _length, float const _lengthZ)
  : pt(_pt),
    depth(_depth),
    normal(_normal),
    length(_length),
    lengthZ(_lengthZ)
{}

RouteVertex const & RouteVertex::operator=(RouteVertex const & v)
{
  if (this != &v)
  {
    pt = v.pt;
    depth = v.depth;
    normal = v.normal;
    length = v.length;
    lengthZ = v.lengthZ;
  }
  return *this;
}

VertexDecl const * RouteVertex::getVertexDecl()
{
  static VertexAttrib attrs [] =
  {
    VertexAttrib(ESemPosition, vertexOffset, EFloat, 3, sizeof(RouteVertex)),
    VertexAttrib(ESemNormal, normalOffset, EFloat, 2, sizeof(RouteVertex)),
    VertexAttrib(ESemLength, lengthOffset, EFloat, 3, sizeof(RouteVertex))
  };

  static VertexDecl vd(attrs, ARRAY_SIZE(attrs));
  return &vd;
}

} // namespace gl
} // namespace graphics

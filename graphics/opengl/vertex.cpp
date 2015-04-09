#include "graphics/opengl/vertex.hpp"
#include "graphics/opengl/opengl.hpp"

#include "base/macros.hpp"

namespace graphics
{
  namespace gl
  {
    Vertex::Vertex()
    {}

    Vertex::Vertex(m2::PointF const & _pt,
                   float _depth,
                   m2::PointF const & _normal,
                   m2::PointF const & _tex)
      : pt(_pt),
        depth(_depth),
        normal(_normal),
        tex(_tex)
    {}

    Vertex::Vertex(Vertex const & v)
      : pt(v.pt),
        depth(v.depth),
        normal(v.normal),
        tex(v.tex)
    {}

    Vertex const & Vertex::operator=(Vertex const & v)
    {
      if (this != &v)
      {
        pt = v.pt;
        depth = v.depth;
        normal = v.normal;
        tex = v.tex;
      }
      return *this;
    }

    VertexDecl const * Vertex::getVertexDecl()
    {
      static VertexAttrib attrs [] =
      {
        VertexAttrib(ESemPosition, vertexOffset, EFloat, 3, sizeof(Vertex)),
        VertexAttrib(ESemNormal, normalOffset, EFloat, 2, sizeof(Vertex)),
        VertexAttrib(ESemTexCoord0, texCoordOffset, EFloat, 2, sizeof(Vertex))
      };

      static VertexDecl vd(attrs, ARRAY_SIZE(attrs));
      return &vd;
    }
  }
}

#include "vertex.hpp"

#include "internal/opengl.hpp"

namespace yg
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

    void Vertex::setupLayout(void * glPtr)
    {
      OGLCHECK(glEnableClientStateFn(GL_VERTEX_ARRAY_MWM));
      OGLCHECK(glVertexPointerFn(3, GL_FLOAT, sizeof(Vertex), (void*)((char*)glPtr + Vertex::vertexOffset)));

      OGLCHECK(glEnableClientStateFn(GL_NORMAL_ARRAY_MWM));
      OGLCHECK(glNormalPointerFn(2, GL_FLOAT, sizeof(Vertex), (void*)((char*)glPtr + Vertex::normalOffset)));

      OGLCHECK(glEnableClientStateFn(GL_TEXTURE_COORD_ARRAY_MWM));
      OGLCHECK(glTexCoordPointerFn(2, GL_FLOAT, sizeof(Vertex), (void*)((char*)glPtr + Vertex::texCoordOffset)));
    }
  }
}

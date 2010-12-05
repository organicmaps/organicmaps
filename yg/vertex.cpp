#include "vertex.hpp"

#include "../base/start_mem_debug.hpp"
#include "internal/opengl.hpp"

namespace yg
{
  namespace gl
  {
    Vertex::Vertex()
    {}

    Vertex::Vertex(m2::PointF const & _pt, float _depth, m2::PointF const & _tex)
      : pt(_pt), depth(_depth), tex(_tex)
    {}

    Vertex::Vertex(Vertex const & v)
      : pt(v.pt), depth(v.depth), tex(v.tex)
    {}

    Vertex const & Vertex::operator=(Vertex const & v)
                                    {
      if (this != &v)
      {
        pt = v.pt;
        depth = v.depth;
        tex = v.tex;
      }
      return *this;
    }

    void Vertex::setupLayout()
    {
      OGLCHECK(glDisableClientState(GL_COLOR_ARRAY));
      OGLCHECK(glEnableClientState(GL_VERTEX_ARRAY));
      OGLCHECK(glVertexPointer(3, GL_FLOAT, sizeof(Vertex), (void*)Vertex::vertexOffset));
      OGLCHECK(glEnable(GL_TEXTURE_2D));
      OGLCHECK(glEnableClientState(GL_TEXTURE_COORD_ARRAY));
      OGLCHECK(glTexCoordPointer(2, GL_FLOAT, sizeof(Vertex), (void*)Vertex::texCoordOffset));
    }
  }
}

#include "../base/SRC_FIRST.hpp"
#include "geometry_renderer.hpp"
#include "base_texture.hpp"
#include "vertexbuffer.hpp"
#include "indexbuffer.hpp"
#include "vertex.hpp"
#include "internal/opengl.hpp"

namespace yg
{
  namespace gl
  {
    GeometryRenderer::GeometryRenderer(base_t::Params const & params) : base_t(params)
    {}

    void GeometryRenderer::drawGeometry(shared_ptr<BaseTexture> const & texture,
                                        shared_ptr<VertexBuffer> const & vertices,
                                        shared_ptr<IndexBuffer> const & indices,
                                        size_t indicesCount)
    {
      vertices->makeCurrent();
      /// it's important to setupLayout after vertices->makeCurrent
      Vertex::setupLayout(vertices->glPtr());
      indices->makeCurrent();

      texture->makeCurrent();

//      OGLCHECK(glPolygonMode( GL_FRONT_AND_BACK, GL_LINE ));

      OGLCHECK(glDrawElements(
        GL_TRIANGLES,
        indicesCount,
        GL_UNSIGNED_SHORT,
        indices->glPtr()));

//      OGLCHECK(glPolygonMode( GL_FRONT_AND_BACK, GL_FILL ));
    }
  }
}

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
    void GeometryRenderer::drawGeometry(shared_ptr<BaseTexture> const & texture,
                                        shared_ptr<VertexBuffer> const & vertices,
                                        shared_ptr<IndexBuffer> const & indices,
                                        size_t indicesCount)
    {
      vertices->makeCurrent();
      Vertex::setupLayout();
      indices->makeCurrent();

      texture->makeCurrent();

      OGLCHECK(glDrawElements(
        GL_TRIANGLES,
        indicesCount,
        GL_UNSIGNED_SHORT,
        0));
    }
  }
}

#include "../base/SRC_FIRST.hpp"
#include "geometry_renderer.hpp"
#include "resource_style.hpp"
#include "base_texture.hpp"
#include "texture.hpp"
#include "vertexbuffer.hpp"
#include "indexbuffer.hpp"
#include "managed_texture.hpp"
#include "vertex.hpp"
#include "internal/opengl.hpp"

#include "../std/bind.hpp"
#include "../base/logging.hpp"

namespace yg
{
  namespace gl
  {
    typedef Texture<DATA_TRAITS, true> TDynamicTexture;

    GeometryRenderer::GeometryRenderer(base_t::Params const & params)
      : base_t(params)
    {}

    void GeometryRenderer::DrawGeometry::perform()
    {
      if (isDebugging())
        LOG(LINFO, ("performing DrawGeometry command"));

      m_vertices->makeCurrent();
      /// it's important to setupLayout after vertices->makeCurrent
      Vertex::setupLayout(m_vertices->glPtr());
      m_indices->makeCurrent();

      m_texture->makeCurrent();

//      OGLCHECK(glPolygonMode( GL_FRONT_AND_BACK, GL_LINE ));

      OGLCHECK(glDrawElementsFn(
        GL_TRIANGLES,
        m_indicesCount,
        GL_UNSIGNED_SHORT,
        m_indices->glPtr()));

//      OGLCHECK(glPolygonMode( GL_FRONT_AND_BACK, GL_FILL ));
    }

    void GeometryRenderer::drawGeometry(shared_ptr<BaseTexture> const & texture,
                                        shared_ptr<VertexBuffer> const & vertices,
                                        shared_ptr<IndexBuffer> const & indices,
                                        size_t indicesCount)
    {
      shared_ptr<DrawGeometry> command(new DrawGeometry());
      command->m_texture = texture;
      command->m_indices = indices;
      command->m_vertices = vertices;
      command->m_indicesCount = indicesCount;

      processCommand(command);
    }

    void GeometryRenderer::applyStates(bool isAntiAliased)
    {
      if (renderQueue())
        return;

      OGLCHECK(glActiveTexture(GL_TEXTURE0));

#ifndef USING_GLSL
      OGLCHECK(glEnableFn(GL_TEXTURE_2D));
#endif

      OGLCHECK(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR));
      OGLCHECK(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR));

      OGLCHECK(glEnableFn(GL_DEPTH_TEST));
      OGLCHECK(glDepthFunc(GL_LEQUAL));

      OGLCHECK(glEnableFn(GL_ALPHA_TEST_MWM));
      OGLCHECK(glAlphaFuncFn(GL_NOTEQUAL, 0.0));

      OGLCHECK(glEnableFn(GL_BLEND));

      if (yg::gl::g_isSeparateBlendFuncSupported)
        OGLCHECK(glBlendFuncSeparateFn(GL_SRC_ALPHA,
                                       GL_ONE_MINUS_SRC_ALPHA,
                                       GL_ZERO,
                                       GL_ONE));
      else
        OGLCHECK(glBlendFunc(GL_SRC_ALPHA,
                             GL_ONE_MINUS_SRC_ALPHA));

#ifndef USING_GLSL
      OGLCHECK(glColor4f(1.0f, 1.0f, 1.0f, 1.0f));
#endif
    }
  }
}

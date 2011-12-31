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

      OGLCHECK(glDrawElements(
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

    void GeometryRenderer::UploadData::perform()
    {
      if (isDebugging())
        LOG(LINFO, ("performing UploadData command"));

      static_cast<gl::ManagedTexture*>(m_texture.get())->lock();

      TDynamicTexture * dynTexture = static_cast<TDynamicTexture*>(m_texture.get());

      for (size_t i = 0; i < m_styles.size(); ++i)
      {
        shared_ptr<ResourceStyle> const & style = m_styles[i];

        TDynamicTexture::view_t v = dynTexture->view(style->m_texRect.SizeX(),
                                                     style->m_texRect.SizeY());

        style->render(&v(0, 0));

        dynTexture->upload(&v(0, 0), style->m_texRect);
      }

      static_cast<gl::ManagedTexture*>(m_texture.get())->unlock();
    }

    void GeometryRenderer::applyStates(bool isAntiAliased)
    {
      if (renderQueue())
        return;

      OGLCHECK(glEnable(GL_TEXTURE_2D));

      OGLCHECK(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR));
      OGLCHECK(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR));

      OGLCHECK(glEnable(GL_DEPTH_TEST));
      OGLCHECK(glDepthFunc(GL_LEQUAL));

      OGLCHECK(glEnable(GL_ALPHA_TEST));
      OGLCHECK(glAlphaFunc(GL_GREATER, 0.0));

      OGLCHECK(glEnable(GL_BLEND));

      if (yg::gl::g_isSeparateBlendFuncSupported)
        OGLCHECK(glBlendFuncSeparateFn(GL_SRC_ALPHA,
                                       GL_ONE_MINUS_SRC_ALPHA,
                                       GL_ZERO,
                                       GL_ONE));
      else
        OGLCHECK(glBlendFunc(GL_SRC_ALPHA,
                             GL_ONE_MINUS_SRC_ALPHA));

      OGLCHECK(glColor4f(1.0f, 1.0f, 1.0f, 1.0f));
    }
  }
}

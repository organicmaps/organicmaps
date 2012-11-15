#include "../base/SRC_FIRST.hpp"
#include "geometry_renderer.hpp"
#include "resource_style.hpp"
#include "base_texture.hpp"
#include "texture.hpp"
#include "buffer_object.hpp"
#include "managed_texture.hpp"
#include "vertex.hpp"
#include "opengl/opengl.hpp"

#include "../std/bind.hpp"
#include "../base/logging.hpp"

namespace graphics
{
  namespace gl
  {
    typedef Texture<DATA_TRAITS, true> TDynamicTexture;

    GeometryRenderer::GeometryRenderer(base_t::Params const & params)
      : base_t(params)
    {}

    GeometryRenderer::UploadData::UploadData(shared_ptr<ResourceStyle> const * styles,
                                             size_t count,
                                             shared_ptr<BaseTexture> const & texture)
      : m_texture(texture)
    {
      m_uploadQueue.reserve(count);
      copy(styles, styles + count, back_inserter(m_uploadQueue));
    }

    GeometryRenderer::UploadData::UploadData()
    {}

    void GeometryRenderer::UploadData::perform()
    {
      if (isDebugging())
        LOG(LINFO, ("performing UploadData command", m_texture->width(), m_texture->height()));

      if (!m_texture)
      {
        LOG(LDEBUG, ("no texture on upload"));
        return;
      }

      if (isDebugging())
        LOG(LINFO, ("uploading to", m_texture->id(), "texture"));

      static_cast<ManagedTexture*>(m_texture.get())->lock();

      TDynamicTexture * dynTexture = static_cast<TDynamicTexture*>(m_texture.get());

      for (size_t i = 0; i < m_uploadQueue.size(); ++i)
      {
        shared_ptr<ResourceStyle> const & style = m_uploadQueue[i];

        TDynamicTexture::view_t v = dynTexture->view(style->m_texRect.SizeX(),
                                                     style->m_texRect.SizeY());

        style->render(&v(0, 0));

        dynTexture->upload(&v(0, 0), style->m_texRect);
      }

      /// In multithreaded resource usage scenarios the suggested way to see
      /// resource update made in one thread to the another thread is
      /// to call the glFlush in thread, which modifies resource and then rebind
      /// resource in another threads that is using this resource, if any.
      OGLCHECK(glFlush());

      static_cast<ManagedTexture*>(m_texture.get())->unlock();
    }

    void GeometryRenderer::UploadData::cancel()
    {
      perform();
    }

    void GeometryRenderer::UploadData::dump()
    {
      m2::RectU r(0, 0, 0, 0);
      if (!m_uploadQueue.empty())
        r = m_uploadQueue[0]->m_texRect;
      LOG(LINFO, ("UploadData: texture", m_texture->id(), ", count=", m_uploadQueue.size(), ", first=", r));
    }


    void GeometryRenderer::uploadStyles(shared_ptr<ResourceStyle> const * styles,
                                        size_t count,
                                        shared_ptr<BaseTexture> const & texture)
    {
      processCommand(make_shared_ptr(new UploadData(styles, count, texture)));
    }

    GeometryRenderer::IMMDrawTexturedRect::IMMDrawTexturedRect(
        m2::RectF const & rect,
        m2::RectF const & texRect,
        shared_ptr<BaseTexture> const & texture,
        shared_ptr<ResourceManager> const & rm)
    {
      m2::PointF rectPoints[4] =
      {
        m2::PointF(rect.minX(), rect.minY()),
        m2::PointF(rect.maxX(), rect.minY()),
        m2::PointF(rect.maxX(), rect.maxY()),
        m2::PointF(rect.minX(), rect.maxY())
      };

      m2::PointF texRectPoints[4] =
      {
        m2::PointF(texRect.minX(), texRect.minY()),
        m2::PointF(texRect.maxX(), texRect.minY()),
        m2::PointF(texRect.maxX(), texRect.maxY()),
        m2::PointF(texRect.minX(), texRect.maxY()),
      };

      m_pts.resize(4);
      m_texPts.resize(4);

      copy(rectPoints, rectPoints + 4, &m_pts[0]);
      copy(texRectPoints, texRectPoints + 4, &m_texPts[0]);
      m_ptsCount = 4;
      m_texture = texture;
      m_hasTexture = true;
      m_hasColor = false;
      m_color = graphics::Color(255, 255, 255, 255);
      m_resourceManager = rm;
    }

    void GeometryRenderer::IMMDrawTexturedPrimitives::perform()
    {
      if (isDebugging())
        LOG(LINFO, ("performing IMMDrawTexturedPrimitives command"));

      graphics::gl::Storage blitStorage = m_resourceManager->blitStorages()->Reserve();

      if (m_resourceManager->blitStorages()->IsCancelled())
      {
        LOG(LDEBUG, ("skipping IMMDrawTexturedPrimitives on cancelled multiBlitStorages pool"));
        return;
      }

      if (!blitStorage.m_vertices->isLocked())
        blitStorage.m_vertices->lock();

      if (!blitStorage.m_indices->isLocked())
        blitStorage.m_indices->lock();

      Vertex * pointsData = (Vertex*)blitStorage.m_vertices->data();

      for (size_t i = 0; i < m_ptsCount; ++i)
      {
        pointsData[i].pt.x = m_pts[i].x;
        pointsData[i].pt.y = m_pts[i].y;
        pointsData[i].depth = graphics::maxDepth;
        pointsData[i].tex.x = m_texPts[i].x;
        pointsData[i].tex.y = m_texPts[i].y;
        pointsData[i].normal.x = 0;
        pointsData[i].normal.y = 0;
      }

      blitStorage.m_vertices->unlock();
      blitStorage.m_vertices->makeCurrent();

      Vertex::setupLayout(blitStorage.m_vertices->glPtr());

      if (m_texture)
        m_texture->makeCurrent();

      unsigned short idxData[4] = {0, 1, 2, 3};
      memcpy(blitStorage.m_indices->data(), idxData, sizeof(idxData));
      blitStorage.m_indices->unlock();
      blitStorage.m_indices->makeCurrent();

      OGLCHECK(glDisableFn(GL_ALPHA_TEST_MWM));
      OGLCHECK(glDisableFn(GL_BLEND));
      OGLCHECK(glDisableFn(GL_DEPTH_TEST));
      OGLCHECK(glDepthMask(GL_FALSE));
      OGLCHECK(glDrawElementsFn(GL_TRIANGLE_FAN, 4, GL_UNSIGNED_SHORT, blitStorage.m_indices->glPtr()));
      OGLCHECK(glDepthMask(GL_TRUE));
      OGLCHECK(glEnableFn(GL_DEPTH_TEST));
      OGLCHECK(glEnableFn(GL_BLEND));
      OGLCHECK(glEnableFn(GL_ALPHA_TEST_MWM));

      blitStorage.m_vertices->discard();
      blitStorage.m_indices->discard();

      m_resourceManager->blitStorages()->Free(blitStorage);
    }

    void GeometryRenderer::DrawGeometry::perform()
    {
      if (isDebugging())
        LOG(LINFO, ("performing DrawGeometry command"));

      m_storage.m_vertices->makeCurrent();
      /// it's crucial to setupLayout after vertices->makeCurrent
      Vertex::setupLayout(m_storage.m_vertices->glPtr());
      m_storage.m_indices->makeCurrent();

      if (isDebugging())
        LOG(LINFO, ("using", m_texture->id(), "texture"));

      if (isDebugging())
        LOG(LINFO, ("drawing", m_indicesCount / 3, "triangles"));

      if (m_texture)
        m_texture->makeCurrent();
      else
        LOG(LINFO, ("null texture used in DrawGeometry"));

//      OGLCHECK(glPolygonMode( GL_FRONT_AND_BACK, GL_LINE ));

      unsigned glPrimType;

      switch (m_primitiveType)
      {
      case ETrianglesFan:
        glPrimType = GL_TRIANGLE_FAN;
        break;
      case ETriangles:
        glPrimType = GL_TRIANGLES;
        break;
      case ETrianglesStrip:
        glPrimType = GL_TRIANGLE_STRIP;
        break;
      };

      OGLCHECK(glDrawElementsFn(
        glPrimType,
        m_indicesCount,
        GL_UNSIGNED_SHORT,
        ((unsigned char*)m_storage.m_indices->glPtr()) + m_indicesOffs));

//      OGLCHECK(glPolygonMode( GL_FRONT_AND_BACK, GL_FILL ));
    }

    void GeometryRenderer::DrawGeometry::dump()
    {
      LOG(LINFO, ("DrawGeometry, texture=", m_texture->id(), ", indicesCount=", m_indicesCount));
    }

    void GeometryRenderer::drawGeometry(shared_ptr<BaseTexture> const & texture,
                                        Storage const & storage,
                                        size_t indicesCount,
                                        size_t indicesOffs,
                                        EPrimitives primType)
    {
      shared_ptr<DrawGeometry> command(new DrawGeometry());

      command->m_texture = texture;
      command->m_storage = storage;
      command->m_indicesCount = indicesCount;
      command->m_indicesOffs = indicesOffs;
      command->m_primitiveType = primType;

      processCommand(command);
    }

    void GeometryRenderer::FreeStorage::perform()
    {
      if (isDebugging())
        LOG(LINFO, ("performing FreeStorage command"));

      if (m_storagePool->IsCancelled())
        return;

      m_storagePool->Free(m_storage);
    }

    void GeometryRenderer::FreeStorage::cancel()
    {
      perform();
    }

    void GeometryRenderer::freeStorage(Storage const & storage, TStoragePool * storagePool)
    {
      shared_ptr<FreeStorage> command(new FreeStorage());

      command->m_storage = storage;
      command->m_storagePool = storagePool;

      processCommand(command);
    }

    void GeometryRenderer::FreeTexture::perform()
    {
      if (isDebugging())
        LOG(LINFO, ("performing FreeTexture command"));

      if (isDebugging())
        LOG(LINFO, ("freeing", m_texture->id(), "texture"));

      if (m_texturePool->IsCancelled())
        return;

      m_texturePool->Free(m_texture);
    }

    void GeometryRenderer::FreeTexture::dump()
    {
      LOG(LINFO, ("FreeTexture, texture=", m_texture->id(), ", pool=", m_texturePool->ResName()));
    }

    void GeometryRenderer::FreeTexture::cancel()
    {
      perform();
    }

    void GeometryRenderer::freeTexture(shared_ptr<BaseTexture> const & texture, TTexturePool * texturePool)
    {
      shared_ptr<FreeTexture> command(new FreeTexture());

      command->m_texture = texture;
      command->m_texturePool = texturePool;

      processCommand(command);
    }

    void GeometryRenderer::UnlockStorage::perform()
    {
      if (isDebugging())
        LOG(LINFO, ("performing UnlockPipeline command"));

      if (m_storage.m_vertices && m_storage.m_indices)
      {
        m_storage.m_vertices->unlock();
        m_storage.m_indices->unlock();

        /// In multithreaded resource usage scenarios the suggested way to see
        /// resource update made in one thread to the another thread is
        /// to call the glFlush in thread, which modifies resource and then rebind
        /// resource in another threads that is using this resource, if any.
        OGLCHECK(glFlush());
      }
      else
        LOG(LDEBUG, ("no storage to unlock"));
    }

    void GeometryRenderer::UnlockStorage::cancel()
    {
      perform();
    }

    void GeometryRenderer::unlockStorage(Storage const & storage)
    {
      shared_ptr<UnlockStorage> command(new UnlockStorage());

      command->m_storage = storage;

      processCommand(command);
    }

    void GeometryRenderer::DiscardStorage::perform()
    {
      if (isDebugging())
        LOG(LINFO, ("performing DiscardStorage command"));

      if (m_storage.m_vertices && m_storage.m_indices)
      {
        m_storage.m_vertices->discard();
        m_storage.m_indices->discard();
      }
      else
        LOG(LDEBUG, ("no storage to discard"));
    }

    void GeometryRenderer::DiscardStorage::cancel()
    {
      perform();
    }

    void GeometryRenderer::discardStorage(Storage const & storage)
    {
      shared_ptr<DiscardStorage> command(new DiscardStorage());

      command->m_storage = storage;

      processCommand(command);
    }

    void GeometryRenderer::ApplyStates::perform()
    {
      if (isDebugging())
        LOG(LINFO, ("performing ApplyStates command"));

      // Disable dither to fix 4-bit textures "grid" issue on Nvidia Tegra cards
      OGLCHECK(glDisableFn(GL_DITHER));

      OGLCHECK(glActiveTexture(GL_TEXTURE0));

      OGLCHECK(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR));
      OGLCHECK(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR));

      OGLCHECK(glEnableFn(GL_DEPTH_TEST));
      OGLCHECK(glDepthFunc(GL_LEQUAL));

      OGLCHECK(glEnableFn(GL_ALPHA_TEST_MWM));
      OGLCHECK(glAlphaFuncFn(GL_NOTEQUAL, 0.0));

      OGLCHECK(glEnableFn(GL_BLEND));
      OGLCHECK(glDepthMask(GL_TRUE));

      if (graphics::gl::g_isSeparateBlendFuncSupported)
        OGLCHECK(glBlendFuncSeparateFn(GL_SRC_ALPHA,
                                       GL_ONE_MINUS_SRC_ALPHA,
                                       GL_ZERO,
                                       GL_ONE));
      else
        OGLCHECK(glBlendFunc(GL_SRC_ALPHA,
                             GL_ONE_MINUS_SRC_ALPHA));

    }

    void GeometryRenderer::ApplyStates::cancel()
    {
      perform();
    }

    void GeometryRenderer::applyStates()
    {
      processCommand(make_shared_ptr(new ApplyStates()));
    }

    void GeometryRenderer::ApplyBlitStates::perform()
    {
      if (isDebugging())
        LOG(LINFO, ("performing ApplyBlitStates command"));

      OGLCHECK(glDisableFn(GL_ALPHA_TEST_MWM));
      OGLCHECK(glDisableFn(GL_BLEND));
      OGLCHECK(glDisableFn(GL_DEPTH_TEST));
      OGLCHECK(glDepthMask(GL_FALSE));
    }

    void GeometryRenderer::ApplyBlitStates::cancel()
    {
      perform();
    }

    void GeometryRenderer::applyBlitStates()
    {
      processCommand(make_shared_ptr(new ApplyBlitStates()));
    }

    void GeometryRenderer::loadMatrix(EMatrix mt,
                                      math::Matrix<float, 4, 4> const & m)
    {
      OGLCHECK(glMatrixModeFn(GL_MODELVIEW_MWM));
      OGLCHECK(glLoadIdentityFn());
      OGLCHECK(glLoadMatrixfFn(&m(0, 0)));
    }
  }
}

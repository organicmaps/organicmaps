#include "geometry_renderer.hpp"
#include "../resource.hpp"
#include "base_texture.hpp"
#include "texture.hpp"
#include "buffer_object.hpp"
#include "managed_texture.hpp"
#include "vertex.hpp"
#include "opengl.hpp"
#include "gl_render_context.hpp"
#include "defines_conv.hpp"

#include "../../base/logging.hpp"
#include "../../base/shared_buffer_manager.hpp"
#include "../../base/math.hpp"
#include "../../std/bind.hpp"

namespace graphics
{
  namespace gl
  {
    typedef Texture<DATA_TRAITS, true> TDynamicTexture;

    GeometryRenderer::GeometryRenderer(base_t::Params const & params)
      : base_t(params)
    {}

    GeometryRenderer::UploadData::UploadData(shared_ptr<Resource> const * resources,
                                             size_t count,
                                             shared_ptr<BaseTexture> const & texture)
      : m_texture(texture)
    {
      m_uploadQueue.reserve(count);
      copy(resources, resources + count, back_inserter(m_uploadQueue));
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

      if (m_uploadQueue.empty())
        return;

      size_t ranges[4];
      ranges[0] = 0;
      ranges[1] = ranges[2] = ranges[3] = m_uploadQueue.size();
      m2::RectU maxRects[3];

      size_t maxIndex = 1;
      m2::RectU currentMaxRect = m_uploadQueue[0]->m_texRect;

      for (size_t i = 1; i < m_uploadQueue.size(); ++i)
      {
        shared_ptr<Resource> const & resource = m_uploadQueue[i];
        if (resource->m_texRect.minY() >= currentMaxRect.maxY())
        {
          ranges[maxIndex] = i;
          if (resource->m_texRect.minX() < currentMaxRect.minX())
          {
            maxRects[maxIndex - 1] = currentMaxRect;
            maxIndex++;
          }
          else
            maxRects[maxIndex - 1].Add(currentMaxRect);

          currentMaxRect = resource->m_texRect;
        }
        else
          currentMaxRect.Add(resource->m_texRect);
      }

      if (ranges[maxIndex] != m_uploadQueue.size())
        maxIndex++;
      maxRects[maxIndex - 1] = currentMaxRect;

      static_cast<ManagedTexture*>(m_texture.get())->lock();

      TDynamicTexture * dynTexture = static_cast<TDynamicTexture*>(m_texture.get());

      for (size_t rangesIt = 1; rangesIt < maxIndex + 1; ++rangesIt)
      {
        m2::RectU const & maxRect = maxRects[rangesIt - 1];
        TDynamicTexture::view_t v = dynTexture->view(maxRect.SizeX(),
                                                     maxRect.SizeY());
        for (size_t resourceIt = ranges[rangesIt - 1]; resourceIt < ranges[rangesIt]; ++resourceIt)
        {
          shared_ptr<Resource> const & resource = m_uploadQueue[resourceIt];
          m2::RectU currentRect = resource->m_texRect;
          currentRect.setMinY(currentRect.minY() - maxRect.minY());
          currentRect.setMaxY(currentRect.maxY() - maxRect.minY());
          currentRect.setMinX(currentRect.minX() - maxRect.minX());
          currentRect.setMaxX(currentRect.maxX() - maxRect.minX());

          size_t renderBufferSize = my::NextPowOf2(currentRect.SizeX() *
                                                   currentRect.SizeY() *
                                                   sizeof(TDynamicTexture::pixel_t));
          SharedBufferManager::shared_buffer_ptr_t buffer = SharedBufferManager::instance().reserveSharedBuffer(renderBufferSize);
          TDynamicTexture::view_t bufferView = gil::interleaved_view(currentRect.SizeX(),
                                                                     currentRect.SizeY(),
                                                                     (TDynamicTexture::pixel_t *)&((*buffer)[0]),
                                                                     currentRect.SizeX() * sizeof(TDynamicTexture::pixel_t));

          resource->render(&bufferView(0, 0));
          TDynamicTexture::view_t subImage = gil::subimage_view(v, currentRect.minX(), currentRect.minY(),
                                                                currentRect.SizeX(), currentRect.SizeY());
          gil::copy_pixels(bufferView, subImage);
          SharedBufferManager::instance().freeSharedBuffer(renderBufferSize, buffer);
        }

        dynTexture->upload(&v(0,0), maxRect);
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


    void GeometryRenderer::uploadResources(shared_ptr<Resource> const * resources,
                                           size_t count,
                                           shared_ptr<BaseTexture> const & texture)
    {
      processCommand(make_shared_ptr(new UploadData(resources, count, texture)));
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

      TStoragePool * storagePool = m_resourceManager->storagePool(ESmallStorage);

      graphics::gl::Storage blitStorage = storagePool->Reserve();

      if (storagePool->IsCancelled())
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
        pointsData[i].depth = 0;
        pointsData[i].tex.x = m_texPts[i].x;
        pointsData[i].tex.y = m_texPts[i].y;
        pointsData[i].normal.x = 0;
        pointsData[i].normal.y = 0;
      }

      blitStorage.m_vertices->unlock();

      gl::RenderContext * rc = static_cast<gl::RenderContext*>(renderContext());
      ProgramManager * pm = rc->programManager();
      shared_ptr<Program> prg = pm->getProgram(EVxTextured, EFrgNoAlphaTest);

      prg->setParam(ESemModelView, rc->matrix(EModelView));
      prg->setParam(ESemProjection, rc->matrix(EProjection));
      prg->setParam(ESemSampler0, 0);

      prg->setStorage(blitStorage);
      prg->setVertexDecl(gl::Vertex::getVertexDecl());
      prg->makeCurrent();

      if (m_texture)
        m_texture->makeCurrent();

      unsigned short idxData[4] = {0, 1, 2, 3};
      memcpy(blitStorage.m_indices->data(), idxData, sizeof(idxData));
      blitStorage.m_indices->unlock();

      OGLCHECK(glDisable(GL_BLEND));
      OGLCHECK(glDisable(GL_DEPTH_TEST));
      OGLCHECK(glDepthMask(GL_FALSE));
      OGLCHECK(glDrawElements(GL_TRIANGLE_FAN,
                              4,
                              GL_UNSIGNED_SHORT,
                              blitStorage.m_indices->glPtr()));

      blitStorage.m_vertices->discard();
      blitStorage.m_indices->discard();

      storagePool->Free(blitStorage);
    }

    void GeometryRenderer::DrawGeometry::perform()
    {
      if (isDebugging())
        LOG(LINFO, ("performing DrawGeometry command"));

      if (isDebugging())
        LOG(LINFO, ("using", m_texture->id(), "texture"));

      if (isDebugging())
        LOG(LINFO, ("drawing", m_indicesCount / 3, "triangles"));

      gl::RenderContext * rc = static_cast<gl::RenderContext*>(renderContext());

      shared_ptr<Program> const & prg = rc->program();

      prg->setParam(ESemModelView, rc->matrix(EModelView));
      prg->setParam(ESemProjection, rc->matrix(EProjection));
      prg->setParam(ESemSampler0, 0);

      prg->setStorage(m_storage);
      prg->setVertexDecl(Vertex::getVertexDecl());
      prg->makeCurrent();

      if (m_texture)
        m_texture->makeCurrent();
      else
        LOG(LINFO, ("null texture used in DrawGeometry"));

      unsigned glPrimType;
      convert(m_primitiveType, glPrimType);

      OGLCHECK(glDrawElements(
        glPrimType,
        m_indicesCount,
        GL_UNSIGNED_SHORT,
        ((unsigned char*)m_storage.m_indices->glPtr()) + m_indicesOffs));
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
      OGLCHECK(glDisable(GL_DITHER));

      OGLCHECK(glActiveTextureFn(GL_TEXTURE0));

      OGLCHECK(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR));
      OGLCHECK(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR));

      OGLCHECK(glEnable(GL_DEPTH_TEST));
      OGLCHECK(glDepthFunc(GL_LEQUAL));

      OGLCHECK(glEnable(GL_BLEND));
      OGLCHECK(glDepthMask(GL_TRUE));

      if (graphics::gl::g_isSeparateBlendFuncSupported)
        OGLCHECK(glBlendFuncSeparateFn(GL_SRC_ALPHA,
                                       GL_ONE_MINUS_SRC_ALPHA,
                                       GL_ZERO,
                                       GL_ONE));
      else
        OGLCHECK(glBlendFunc(GL_SRC_ALPHA,
                             GL_ONE_MINUS_SRC_ALPHA));

      /// Applying program
      gl::RenderContext * rc = static_cast<gl::RenderContext*>(renderContext());
      ProgramManager * pm = rc->programManager();
      shared_ptr<Program> prg = pm->getProgram(EVxTextured, EFrgAlphaTest);

      prg->setParam(ESemModelView, rc->matrix(EModelView));
      prg->setParam(ESemProjection, rc->matrix(EProjection));
      prg->setParam(ESemSampler0, 0);

      rc->setProgram(prg);
    }

    void GeometryRenderer::applyStates()
    {
      processCommand(make_shared_ptr(new ApplyStates()));
    }

    void GeometryRenderer::ApplyBlitStates::perform()
    {
      if (isDebugging())
        LOG(LINFO, ("performing ApplyBlitStates command"));

      OGLCHECK(glDisable(GL_DEPTH_TEST));
      OGLCHECK(glDepthMask(GL_FALSE));

      /// Applying program
      gl::RenderContext * rc = static_cast<gl::RenderContext*>(renderContext());
      ProgramManager * pm = rc->programManager();
      shared_ptr<Program> prg = pm->getProgram(EVxTextured, EFrgNoAlphaTest);

      prg->setParam(ESemModelView, rc->matrix(EModelView));
      prg->setParam(ESemProjection, rc->matrix(EProjection));
      prg->setParam(ESemSampler0, 0);

      rc->setProgram(prg);
    }

    void GeometryRenderer::applyBlitStates()
    {
      processCommand(make_shared_ptr(new ApplyBlitStates()));
    }

    void GeometryRenderer::ApplySharpStates::perform()
    {
      if (isDebugging())
        LOG(LINFO, ("performing ApplySharpStates command"));

      // Disable dither to fix 4-bit textures "grid" issue on Nvidia Tegra cards
      OGLCHECK(glDisable(GL_DITHER));

      OGLCHECK(glActiveTextureFn(GL_TEXTURE0));

      OGLCHECK(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR));
      OGLCHECK(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR));

      OGLCHECK(glEnable(GL_DEPTH_TEST));
      OGLCHECK(glDepthFunc(GL_LEQUAL));

      OGLCHECK(glEnable(GL_BLEND));
      OGLCHECK(glDepthMask(GL_TRUE));

      if (graphics::gl::g_isSeparateBlendFuncSupported)
        OGLCHECK(glBlendFuncSeparateFn(GL_SRC_ALPHA,
                                       GL_ONE_MINUS_SRC_ALPHA,
                                       GL_ZERO,
                                       GL_ONE));
      else
        OGLCHECK(glBlendFunc(GL_SRC_ALPHA,
                             GL_ONE_MINUS_SRC_ALPHA));

      /// Applying program
      gl::RenderContext * rc = static_cast<gl::RenderContext*>(renderContext());
      ProgramManager * pm = rc->programManager();
      shared_ptr<Program> prg = pm->getProgram(EVxSharp, EFrgAlphaTest);

      prg->setParam(ESemModelView, rc->matrix(EModelView));
      prg->setParam(ESemProjection, rc->matrix(EProjection));
      prg->setParam(ESemSampler0, 0);

      rc->setProgram(prg);
    }

    void GeometryRenderer::applySharpStates()
    {
      processCommand(make_shared_ptr(new ApplySharpStates()));
    }
  }
}

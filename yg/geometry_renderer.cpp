#include "../base/SRC_FIRST.hpp"
#include "geometry_renderer.hpp"
#include "resource_style.hpp"
#include "base_texture.hpp"
#include "texture.hpp"
#include "buffer_object.hpp"
#include "managed_texture.hpp"
#include "display_list.hpp"
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
      : base_t(params),
        m_displayList(0)
    {}

    GeometryRenderer::UploadData::UploadData(SkinPage::TUploadQueue const & uploadQueue,
                                             shared_ptr<BaseTexture> const & texture)
      : m_uploadQueue(uploadQueue), m_texture(texture)
    {}

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


    void GeometryRenderer::uploadTextureImpl(SkinPage::TUploadQueue const & uploadQueue,
                                             size_t start, size_t end,
                                             shared_ptr<BaseTexture> const & texture,
                                             bool shouldAddCheckPoint)
    {
      vector<shared_ptr<ResourceStyle> > v;
      v.reserve(end - start);
      copy(&uploadQueue[0] + start, &uploadQueue[0] + end, back_inserter(v));

      shared_ptr<UploadData> command(new UploadData(v, texture));

      if (m_displayList)
        m_displayList->uploadData(command);
      else
        processCommand(command);

      if (shouldAddCheckPoint)
      {
        if (m_displayList)
          m_displayList->addCheckPoint();
        else
          addCheckPoint();
      }
    }

    void GeometryRenderer::uploadTexture(SkinPage::TUploadQueue const & uploadQueue,
                                         shared_ptr<BaseTexture> const & texture)
    {
      /// splitting the whole queue of commands into the chunks no more
      /// than 100kb of uploadable data each

      /// tracking the number of bytes downloaded onto the texture
      /// in a single shot.
      size_t bytesUploaded = 0;
      size_t bytesPerPixel = yg::formatSize(resourceManager()->params().m_texFormat);
      size_t prev = 0;

      for (size_t i = 0; i < uploadQueue.size(); ++i)
      {
        shared_ptr<ResourceStyle> const & style = uploadQueue[i];

        bytesUploaded += style->m_texRect.SizeX() * style->m_texRect.SizeY() * bytesPerPixel;

        if (bytesUploaded > 64 * 1024)
        {
          uploadTextureImpl(uploadQueue, prev, i + 1, texture, true);
          prev = i + 1;
          bytesUploaded = 0;
        }
      }

      if (!uploadQueue.empty())
      {
        uploadTextureImpl(uploadQueue, prev, uploadQueue.size(), texture, false);
        bytesUploaded = 0;
      }
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

      OGLCHECK(glDrawElementsFn(
        m_primitiveType,
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
                                        unsigned primType)
    {
      shared_ptr<DrawGeometry> command(new DrawGeometry());

      command->m_texture = texture;
      command->m_storage = storage;
      command->m_indicesCount = indicesCount;
      command->m_indicesOffs = indicesOffs;
      command->m_primitiveType = primType;

      if (m_displayList)
        m_displayList->drawGeometry(command);
      else
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

      if (m_displayList)
        m_displayList->freeStorage(command);
      else
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

      if (m_displayList)
        m_displayList->freeTexture(command);
      else
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

      if (m_displayList)
        m_displayList->unlockStorage(command);
      else
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

      if (m_displayList)
        m_displayList->discardStorage(command);
      else
        processCommand(command);
    }

    DisplayList * GeometryRenderer::createDisplayList()
    {
      return new DisplayList(this);
    }

    void GeometryRenderer::setDisplayList(DisplayList * displayList)
    {
      m_displayList = displayList;
    }

    DisplayList * GeometryRenderer::displayList() const
    {
      return m_displayList;
    }

    void GeometryRenderer::ApplyStates::perform()
    {
      if (isDebugging())
        LOG(LINFO, ("performing ApplyStates command"));

      // Disable dither to fix 4-bit textures "grid" issue on Nvidia Tegra cards
      OGLCHECK(glDisableFn(GL_DITHER));

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
      OGLCHECK(glDepthMask(GL_TRUE));

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

    void GeometryRenderer::ApplyStates::cancel()
    {
      perform();
    }

    void GeometryRenderer::applyStates()
    {

      shared_ptr<ApplyStates> command(new ApplyStates());

      if (m_displayList)
        m_displayList->applyStates(command);
      else
        processCommand(command);
    }

    void GeometryRenderer::ApplyBlitStates::perform()
    {
      if (isDebugging())
        LOG(LINFO, ("performing ApplyBlitStates command"));

#ifndef USING_GLSL
      OGLCHECK(glEnable(GL_TEXTURE_2D));
#endif

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
      shared_ptr<ApplyBlitStates> command(new ApplyBlitStates());

      if (m_displayList)
        m_displayList->applyBlitStates(command);
      else
        processCommand(command);
    }

  }
}

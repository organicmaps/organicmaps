#include "graphics/opengl/geometry_renderer.hpp"
#include "graphics/resource.hpp"
#include "graphics/opengl/base_texture.hpp"
#include "graphics/opengl/texture.hpp"
#include "graphics/opengl/buffer_object.hpp"
#include "graphics/opengl/managed_texture.hpp"
#include "graphics/opengl/vertex.hpp"
#include "graphics/opengl/opengl.hpp"
#include "graphics/opengl/gl_render_context.hpp"
#include "graphics/opengl/defines_conv.hpp"
#include "graphics/opengl/route_vertex.hpp"

#include "base/logging.hpp"
#include "base/shared_buffer_manager.hpp"
#include "base/math.hpp"
#include "std/bind.hpp"

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
      if (!m_texture)
      {
        LOG(LDEBUG, ("no texture on upload"));
        return;
      }

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
      OGLCHECK(glFlushFn());

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
      LOG(LDEBUG, ("UploadData: texture", m_texture->id(), ", count=", m_uploadQueue.size(), ", first=", r));
    }


    void GeometryRenderer::uploadResources(shared_ptr<Resource> const * resources,
                                           size_t count,
                                           shared_ptr<BaseTexture> const & texture)
    {
      processCommand(make_shared<UploadData>(resources, count, texture));
    }

    GeometryRenderer::DrawGeometry::DrawGeometry()
      : m_alfa(1.0)
    {
    }

    bool GeometryRenderer::DrawGeometry::isNeedAdditionalUniforms() const
    {
      gl::RenderContext const * rc = static_cast<gl::RenderContext const *>(renderContext());

      shared_ptr<Program> const & prg = rc->program();
      return prg->isParamExist(ETransparency);
    }

    void GeometryRenderer::DrawGeometry::setAdditionalUniforms(const UniformsHolder & holder)
    {
      VERIFY(holder.getValue(ETransparency, m_alfa), ());
    }

    void GeometryRenderer::DrawGeometry::resetAdditionalUniforms()
    {
      m_alfa = 1.0;
    }

    void GeometryRenderer::DrawGeometry::perform()
    {
      gl::RenderContext * rc = static_cast<gl::RenderContext*>(renderContext());

      shared_ptr<Program> const & prg = rc->program();

      prg->setParam(ESemModelView, rc->matrix(EModelView));
      prg->setParam(ESemProjection, rc->matrix(EProjection));
      prg->setParam(ESemSampler0, 0);

      if (prg->isParamExist(ETransparency))
        prg->setParam(ETransparency, m_alfa);

      prg->setStorage(m_storage);
      prg->setVertexDecl(Vertex::getVertexDecl());

      /// When the binders leave the scope the buffer object will be unbound.
      gl::BufferObject::Binder verticesBufBinder, indicesBufBinder;
      prg->makeCurrent(verticesBufBinder, indicesBufBinder);

      if (m_texture)
        m_texture->makeCurrent();
      else
        LOG(LDEBUG, ("null texture used in DrawGeometry"));

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
      LOG(LDEBUG, ("DrawGeometry, texture=", m_texture->id(), ", indicesCount=", m_indicesCount));
    }

    GeometryRenderer::DrawRouteGeometry::DrawRouteGeometry()
      : m_indicesCount(0)
    {
      ResetUniforms();
    }

    bool GeometryRenderer::DrawRouteGeometry::isNeedAdditionalUniforms() const
    {
      return true;
    }

    void GeometryRenderer::DrawRouteGeometry::setAdditionalUniforms(UniformsHolder const & holder)
    {
      holder.getValue(ERouteHalfWidth, m_halfWidth[0], m_halfWidth[1]);
      holder.getValue(ERouteColor, m_color[0], m_color[1], m_color[2], m_color[3]);
      holder.getValue(ERouteClipLength, m_clipLength);
      holder.getValue(ERouteTextureRect, m_textureRect[0], m_textureRect[1], m_textureRect[2], m_textureRect[3]);
    }

    void GeometryRenderer::DrawRouteGeometry::ResetUniforms()
    {
      m_halfWidth[0] = 0.0f;
      m_halfWidth[1] = 0.0f;

      m_color[0] = 0.0f;
      m_color[1] = 0.0f;
      m_color[2] = 0.0f;
      m_color[3] = 0.0f;

      m_clipLength = 0.0f;

      m_textureRect[0] = 0.0f;
      m_textureRect[1] = 0.0f;
      m_textureRect[2] = 0.0f;
      m_textureRect[3] = 0.0f;

      m_arrowBorders = math::Zero<float, 4>();
    }

    void GeometryRenderer::DrawRouteGeometry::resetAdditionalUniforms()
    {
      ResetUniforms();
    }

    bool GeometryRenderer::DrawRouteGeometry::isNeedIndicesCount() const
    {
      return true;
    }

    void GeometryRenderer::DrawRouteGeometry::setIndicesCount(size_t indicesCount)
    {
      m_indicesCount = indicesCount;
    }

    void GeometryRenderer::DrawRouteGeometry::perform()
    {
      gl::RenderContext * rc = static_cast<gl::RenderContext*>(renderContext());

      shared_ptr<Program> const & prg = rc->program();

      prg->setParam(ESemModelView, rc->matrix(EModelView));
      prg->setParam(ESemProjection, rc->matrix(EProjection));
      prg->setParam(ERouteHalfWidth, m_halfWidth[0], m_halfWidth[1]);

      if (prg->isParamExist(ESemSampler0))
        prg->setParam(ESemSampler0, 0);

      if (prg->isParamExist(ERouteColor))
        prg->setParam(ERouteColor, m_color[0], m_color[1], m_color[2], m_color[3]);

      if (prg->isParamExist(ERouteClipLength))
        prg->setParam(ERouteClipLength, m_clipLength);

      if (prg->isParamExist(ERouteTextureRect))
        prg->setParam(ERouteTextureRect, m_textureRect[0], m_textureRect[1], m_textureRect[2], m_textureRect[3]);

      prg->setStorage(m_storage);
      prg->setVertexDecl(RouteVertex::getVertexDecl());

      /// When the binders leave the scope the buffer object will be unbound.
      gl::BufferObject::Binder verticesBufBinder, indicesBufBinder;
      prg->makeCurrent(verticesBufBinder, indicesBufBinder);

      if (m_texture)
        m_texture->makeCurrent();
      else
        LOG(LDEBUG, ("null texture used in DrawGeometry"));

      OGLCHECK(glDrawElements(
        GL_TRIANGLES,
        m_indicesCount,
        GL_UNSIGNED_SHORT,
        (unsigned char*)m_storage.m_indices->glPtr()));
    }

    void GeometryRenderer::DrawRouteGeometry::dump()
    {
      LOG(LDEBUG, ("DrawRouteGeometry, texture=", m_texture->id(), ", indicesCount=", m_indicesCount));
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

    void GeometryRenderer::drawRouteGeometry(shared_ptr<BaseTexture> const & texture,
                                             Storage const & storage)
    {
      shared_ptr<DrawRouteGeometry> command(new DrawRouteGeometry());

      command->m_texture = texture;
      command->m_storage = storage;

      processCommand(command);
    }

    void GeometryRenderer::clearRouteGeometry()
    {
      gl::RenderContext * rc = static_cast<gl::RenderContext*>(renderContext());
      ProgramManager * pm = rc->programManager();

      shared_ptr<Program> prg = pm->getProgram(EVxRoute, EFrgRoute);
      prg->setStorage(gl::Storage());

      prg = pm->getProgram(EVxRoute, EFrgRouteArrow);
      prg->setStorage(gl::Storage());
    }

    void GeometryRenderer::FreeStorage::perform()
    {
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
      if (m_texturePool->IsCancelled())
        return;

      m_texturePool->Free(m_texture);
    }

    void GeometryRenderer::FreeTexture::dump()
    {
      LOG(LDEBUG, ("FreeTexture, texture=", m_texture->id(), ", pool=", m_texturePool->ResName()));
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
      if (m_storage.m_vertices && m_storage.m_indices)
      {
        m_storage.m_vertices->unlock();
        m_storage.m_indices->unlock();

        /// In multithreaded resource usage scenarios the suggested way to see
        /// resource update made in one thread to the another thread is
        /// to call the glFlush in thread, which modifies resource and then rebind
        /// resource in another threads that is using this resource, if any.
        OGLCHECK(glFlushFn());
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
      if (m_type == AlfaVaringProgram)
        prg = pm->getProgram(EVxTextured, EFrgVarAlfa);
      else if (m_type == RouteProgram)
        prg = pm->getProgram(EVxRoute, EFrgRoute);
      else if (m_type == RouteArrowProgram)
        prg = pm->getProgram(EVxRoute, EFrgRouteArrow);

      if (m_type == DefaultProgram || m_type == AlfaVaringProgram)
      {
        prg->setParam(ESemModelView, rc->matrix(EModelView));
        prg->setParam(ESemProjection, rc->matrix(EProjection));
        prg->setParam(ESemSampler0, 0);
      }

      rc->setProgram(prg);
    }

    void GeometryRenderer::applyStates()
    {
      processCommand(make_shared<ApplyStates>());
    }

    void GeometryRenderer::applyVarAlfaStates()
    {
      processCommand(make_shared<ApplyStates>(ApplyStates::AlfaVaringProgram));
    }

    void GeometryRenderer::applyRouteStates()
    {
      processCommand(make_shared<ApplyStates>(ApplyStates::RouteProgram));
    }

    void GeometryRenderer::applyRouteArrowStates()
    {
      processCommand(make_shared<ApplyStates>(ApplyStates::RouteArrowProgram));
    }

    void GeometryRenderer::ApplyBlitStates::perform()
    {
      OGLCHECK(glEnable(GL_DEPTH_TEST));
      OGLCHECK(glDepthFunc(GL_LEQUAL));
      OGLCHECK(glDepthMask(GL_TRUE));

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
      processCommand(make_shared<ApplyBlitStates>());
    }

    void GeometryRenderer::ApplySharpStates::perform()
    {
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
      processCommand(make_shared<ApplySharpStates>());
    }
  }
}

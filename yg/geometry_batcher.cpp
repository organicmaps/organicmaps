#include "geometry_batcher.hpp"
#include "skin.hpp"
#include "color.hpp"
#include "utils.hpp"
#include "resource_manager.hpp"
#include "skin_page.hpp"
#include "base_texture.hpp"

#include "internal/opengl.hpp"

#include "../geometry/rect2d.hpp"

#include "../base/assert.hpp"
#include "../base/math.hpp"
#include "../base/mutex.hpp"
#include "../base/logging.hpp"

#include "../std/algorithm.hpp"
#include "../std/bind.hpp"

namespace yg
{
  namespace gl
  {
    GeometryBatcher::Params::Params()
      : m_useGuiResources(false)
    {}

    GeometryBatcher::GeometryBatcher(Params const & params)
      : base_t(params),
        m_isAntiAliased(true),
        m_useGuiResources(params.m_useGuiResources)
    {
      reset(-1);
      base_t::applyStates();

      /// 1 to turn antialiasing on
      /// 2 to switch it off
      m_aaShift = m_isAntiAliased ? 1 : 2;
    }

   GeometryBatcher::~GeometryBatcher()
   {
     for (size_t i = 0; i < m_pipelines.size(); ++i)
     {
       discardPipeline(i);
       freePipeline(i);
       if (m_skin->getPage(i)->type() != SkinPage::EStatic)
         freeTexture(i);
     }
   }

   void GeometryBatcher::reset(int pipelineID)
   {
     for (size_t i = 0; i < m_pipelines.size(); ++i)
     {
       if ((pipelineID == -1) || ((size_t)pipelineID == i))
       {
         m_pipelines[i].m_currentVertex = 0;
         m_pipelines[i].m_currentIndex = 0;
       }
     }
   }

   void GeometryBatcher::GeometryPipeline::checkStorage(shared_ptr<ResourceManager> const & resourceManager) const
   {
     if (!m_hasStorage)
     {
       if (m_useGuiResources)
         m_storage = resourceManager->guiThreadStorages()->Reserve();
       else
       {
         switch (m_type)
         {
         case SkinPage::EPrimary:
           m_storage = resourceManager->primaryStorages()->Reserve();
           break;
         case SkinPage::EFonts:
           m_storage = resourceManager->smallStorages()->Reserve();
           break;
         case SkinPage::EStatic:
           m_storage = resourceManager->smallStorages()->Reserve();
           break;
         default:
           LOG(LERROR, ("invalid storage type in checkStorage"));
         }
       }

       if (m_storage.m_vertices && m_storage.m_indices)
       {
         m_maxVertices = m_storage.m_vertices->size() / sizeof(Vertex);
         m_maxIndices = m_storage.m_indices->size() / sizeof(unsigned short);

         if (!m_storage.m_vertices->isLocked())
           m_storage.m_vertices->lock();
         if (!m_storage.m_indices->isLocked())
           m_storage.m_indices->lock();

         m_vertices = (Vertex*)m_storage.m_vertices->data();
         m_indices = (unsigned short *)m_storage.m_indices->data();

         m_hasStorage = true;
       }
       else
       {
         m_maxVertices = 0;
         m_maxIndices = 0;

         m_vertices = 0;
         m_indices = 0;

         m_hasStorage = false;
       }
     }
   }

   void GeometryBatcher::freePipeline(int pipelineID)
   {
     GeometryPipeline & pipeline = m_pipelines[pipelineID];

     if (pipeline.m_hasStorage)
     {
       TStoragePool * storagePool = 0;
       if (pipeline.m_useGuiResources)
         storagePool = resourceManager()->guiThreadStorages();
       else
         switch (pipeline.m_type)
         {
         case SkinPage::EPrimary:
           storagePool = resourceManager()->primaryStorages();
           break;
         case SkinPage::EFonts:
           storagePool = resourceManager()->smallStorages();
           break;
         case SkinPage::EStatic:
           storagePool = resourceManager()->smallStorages();
           break;
         default:
           LOG(LERROR, ("invalid pipeline type in freePipeline"));
           break;
         }

       base_t::freeStorage(pipeline.m_storage, storagePool);

       pipeline.m_hasStorage = false;
       pipeline.m_storage = Storage();
     }
   }

   void GeometryBatcher::setAdditionalSkinPage(shared_ptr<SkinPage> const & p)
   {
     if (m_skin != 0)
     {
       m_skin->setAdditionalPage(p);
       int pagesCount = m_skin->getPagesCount();
       m_pipelines.resize(pagesCount + 1);

       /// additional page are fixed-content page, and shouldn't be modified by this screen.
       /*m_skin->addOverflowFn(bind(&GeometryBatcher::flush, this, _1), 100);

       m_skin->addClearPageFn(bind(&GeometryBatcher::flush, this, _1), 100);
       m_skin->addClearPageFn(bind(&GeometryBatcher::freeTexture, this, _1), 99);*/

       for (size_t i = 0; i < 1; ++i)
       {
         m_pipelines[i + pagesCount].m_useGuiResources = m_useGuiResources;
         m_pipelines[i + pagesCount].m_currentVertex = 0;
         m_pipelines[i + pagesCount].m_currentIndex = 0;

         m_pipelines[i + pagesCount].m_hasStorage = false;
         m_pipelines[i + pagesCount].m_type = p->type();

         m_pipelines[i + pagesCount].m_maxVertices = 0;
         m_pipelines[i + pagesCount].m_maxIndices = 0;

         m_pipelines[i + pagesCount].m_vertices = 0;
         m_pipelines[i + pagesCount].m_indices = 0;
       }
     }
   }

   void GeometryBatcher::clearAdditionalSkinPage()
   {
     if (m_skin != 0)
     {
       size_t pagesCount = m_skin->getPagesCount();
       size_t additionalPagesCount = m_skin->getAdditionalPagesCount();

       m_skin->clearAdditionalPage();

       for (unsigned i = pagesCount; i < pagesCount + additionalPagesCount; ++i)
         freePipeline(i);

       m_pipelines.resize(m_skin->getPagesCount());
     }
   }

   void GeometryBatcher::setSkin(shared_ptr<Skin> skin)
   {
     m_skin = skin;
     if (m_skin != 0)
     {
       /// settings proper skin page type according to useGuiResources flag
       if (m_useGuiResources)
         for (size_t i = 0; i < m_skin->getPagesCount(); ++i)
           if (m_skin->getPage(i)->type() != SkinPage::EStatic)
             m_skin->getPage(i)->setType(SkinPage::ELightWeight);

       m_pipelines.resize(m_skin->getPagesCount());

       m_skin->addOverflowFn(bind(&GeometryBatcher::flush, this, _1), 100);

       m_skin->addClearPageFn(bind(&GeometryBatcher::flush, this, _1), 100);
       m_skin->addClearPageFn(bind(&GeometryBatcher::freeTexture, this, _1), 99);

       for (size_t i = 0; i < m_pipelines.size(); ++i)
       {
         m_pipelines[i].m_useGuiResources = m_useGuiResources;
         m_pipelines[i].m_currentVertex = 0;
         m_pipelines[i].m_currentIndex = 0;

         m_pipelines[i].m_hasStorage = false;
         m_pipelines[i].m_type = skin->getPage(i)->type();

         m_pipelines[i].m_maxVertices = 0;
         m_pipelines[i].m_maxIndices = 0;

         m_pipelines[i].m_vertices = 0;
         m_pipelines[i].m_indices = 0;
       }
     }
   }

   shared_ptr<Skin> const & GeometryBatcher::skin() const
   {
     return m_skin;
   }

   void GeometryBatcher::beginFrame()
   {
     base_t::beginFrame();
     reset(-1);
     for (size_t i = 0; i < m_pipelines.size(); ++i)
     {
       m_pipelines[i].m_verticesDrawn = 0;
       m_pipelines[i].m_indicesDrawn = 0;
     }
   }

   void GeometryBatcher::clear(yg::Color const & c, bool clearRT, float depth, bool clearDepth)
   {
     flush(-1);
     base_t::clear(c, clearRT, depth, clearDepth);
   }

   void GeometryBatcher::setRenderTarget(shared_ptr<RenderTarget> const & rt)
   {
     flush(-1);
     base_t::setRenderTarget(rt);
   }

   void GeometryBatcher::endFrame()
   {
     flush(-1);
     /// Syncronization point.
     enableClipRect(false);

     if (isDebugging())
     {
       for (size_t i = 0; i < m_pipelines.size(); ++i)
         if ((m_pipelines[i].m_verticesDrawn != 0) || (m_pipelines[i].m_indicesDrawn != 0))
         {
           LOG(LINFO, ("pipeline #", i, " vertices=", m_pipelines[i].m_verticesDrawn, ", triangles=", m_pipelines[i].m_indicesDrawn / 3));
         }
     }

     base_t::endFrame();
   }

   bool GeometryBatcher::hasRoom(size_t verticesCount, size_t indicesCount, int pipelineID) const
   {
     GeometryPipeline const & pipeline = m_pipelines[pipelineID];

     pipeline.checkStorage(resourceManager());
     if (!pipeline.m_hasStorage)
       return false;

     return ((pipeline.m_currentVertex + verticesCount <= pipeline.m_maxVertices)
         &&  (pipeline.m_currentIndex + indicesCount <= pipeline.m_maxIndices));
   }

   int GeometryBatcher::verticesLeft(int pipelineID) const
   {
     GeometryPipeline const & pipeline = m_pipelines[pipelineID];

     pipeline.checkStorage(resourceManager());
     if (!pipeline.m_hasStorage)
       return -1;

     return pipeline.m_maxVertices - pipeline.m_currentVertex;
   }

   int GeometryBatcher::indicesLeft(int pipelineID) const
   {
     GeometryPipeline const & pipeline = m_pipelines[pipelineID];

     pipeline.checkStorage(resourceManager());
     if (!pipeline.m_hasStorage)
       return -1;

     return pipeline.m_maxIndices - pipeline.m_currentIndex;
   }

   void GeometryBatcher::flush(int pipelineID)
   {
     if (m_skin)
     {
       for (size_t i = m_pipelines.size(); i > 0; --i)
       {
         if ((pipelineID == -1) || ((i - 1) == (size_t)pipelineID))
         {
           flushPipeline(m_skin->getPage(i - 1), i - 1);
           reset(i - 1);
         }
       }
     }
   }

   void GeometryBatcher::freeTexture(int pipelineID)
   {
     if (!m_skin->getPage(pipelineID)->hasTexture())
       return;

     shared_ptr<BaseTexture> texture = m_skin->getPage(pipelineID)->texture();
     TTexturePool * texturePool = 0;

     switch (m_skin->getPage(pipelineID)->type())
     {
     case SkinPage::EPrimary:
       texturePool = resourceManager()->primaryTextures();
       break;
     case SkinPage::EFonts:
       texturePool = resourceManager()->fontTextures();
       break;
     case SkinPage::ELightWeight:
       texturePool = resourceManager()->guiThreadTextures();
       break;
     case SkinPage::EStatic:
       LOG(LWARNING, ("texture with EStatic can't be freed."));
       return;
     }

     base_t::freeTexture(texture, texturePool);

     m_skin->getPage(pipelineID)->resetTexture();
   }

   void GeometryBatcher::unlockPipeline(int pipelineID)
   {
     GeometryPipeline & pipeline = m_pipelines[pipelineID];
     base_t::unlockStorage(pipeline.m_storage);
   }

   void GeometryBatcher::discardPipeline(int pipelineID)
   {
     GeometryPipeline & pipeline = m_pipelines[pipelineID];

     if (pipeline.m_hasStorage)
       base_t::discardStorage(pipeline.m_storage);
   }

   void GeometryBatcher::flushPipeline(shared_ptr<SkinPage> const & skinPage,
                                       int pipelineID)
   {
     GeometryPipeline & pipeline = m_pipelines[pipelineID];
     if (pipeline.m_currentIndex)
     {
       if (skinPage->hasData())
       {
         uploadTexture(skinPage->uploadQueue(), skinPage->texture());
         skinPage->clearUploadQueue();
       }

       unlockPipeline(pipelineID);

       drawGeometry(skinPage->texture(),
                    pipeline.m_storage.m_vertices,
                    pipeline.m_storage.m_indices,
                    pipeline.m_currentIndex,
                    0,
                    GL_TRIANGLES);

       discardPipeline(pipelineID);


       if (isDebugging())
       {
         pipeline.m_verticesDrawn += pipeline.m_currentVertex;
         pipeline.m_indicesDrawn += pipeline.m_currentIndex;
//               LOG(LINFO, ("Pipeline #", i - 1, "draws ", pipeline.m_currentIndex / 3, "/", pipeline.m_maxIndices / 3," triangles"));
       }

       freePipeline(pipelineID);

       pipeline.m_maxIndices = 0;
       pipeline.m_maxVertices = 0;
       pipeline.m_vertices = 0;
       pipeline.m_indices = 0;
       pipeline.m_currentIndex = 0;
       pipeline.m_currentVertex = 0;
     }
   }

   void GeometryBatcher::drawTexturedPolygon(
       m2::PointD const & ptShift,
       ang::AngleD const & angle,
       float tx0, float ty0, float tx1, float ty1,
       float x0, float y0, float x1, float y1,
       double depth,
       int pipelineID)
   {
     if (!hasRoom(4, 6, pipelineID))
       flush(pipelineID);

     m_pipelines[pipelineID].checkStorage(resourceManager());
     if (!m_pipelines[pipelineID].m_hasStorage)
       return;

     float texMinX = tx0;
     float texMaxX = tx1;
     float texMinY = ty0;
     float texMaxY = ty1;

     shared_ptr<BaseTexture> const & texture = m_skin->getPage(pipelineID)->texture();

     if (!texture)
     {
       LOG(LDEBUG, ("returning as no texture is reserved"));
       return;
     }

     texture->mapPixel(texMinX, texMinY);
     texture->mapPixel(texMaxX, texMaxY);

     /// rotated and translated four points (x0, y0), (x0, y1), (x1, y1), (x1, y0)

     m2::PointF coords[4] =
     {
       m2::PointF(x0 * angle.cos() - y0 * angle.sin() + ptShift.x, x0 * angle.sin() + y0 * angle.cos() + ptShift.y),
       m2::PointF(x0 * angle.cos() - y1 * angle.sin() + ptShift.x, x0 * angle.sin() + y1 * angle.cos() + ptShift.y),
       m2::PointF(x1 * angle.cos() - y1 * angle.sin() + ptShift.x, x1 * angle.sin() + y1 * angle.cos() + ptShift.y),
       m2::PointF(x1 * angle.cos() - y0 * angle.sin() + ptShift.x, x1 * angle.sin() + y0 * angle.cos() + ptShift.y)
     };

     /// Special case. Making straight fonts sharp.
     if (angle.val() == 0)
     {
       float deltaX = coords[0].x - ceil(coords[0].x);
       float deltaY = coords[0].y - ceil(coords[0].y);

       for (size_t i = 0; i < 4; ++i)
       {
         coords[i].x -= deltaX;
         coords[i].y -= deltaY;
       }
     }

     m2::PointF texCoords[4] =
     {
       m2::PointF(texMinX, texMinY),
       m2::PointF(texMinX, texMaxY),
       m2::PointF(texMaxX, texMaxY),
       m2::PointF(texMaxX, texMinY)
     };

     addTexturedFan(coords, texCoords, 4, depth, pipelineID);
   }

   void GeometryBatcher::addTexturedFan(m2::PointF const * coords, m2::PointF const * texCoords, unsigned size, double depth, int pipelineID)
   {
     if (!hasRoom(size, (size - 2) * 3, pipelineID))
       flush(pipelineID);

     GeometryPipeline & pipeline = m_pipelines[pipelineID];

     pipeline.checkStorage(resourceManager());
     if (!pipeline.m_hasStorage)
       return;

     ASSERT(size > 2, ());

     size_t vOffset = pipeline.m_currentVertex;
     size_t iOffset = pipeline.m_currentIndex;

     for (unsigned i = 0; i < size; ++i)
     {
       pipeline.m_vertices[vOffset + i].pt = coords[i];
       pipeline.m_vertices[vOffset + i].tex = texCoords[i];
       pipeline.m_vertices[vOffset + i].depth = depth;
     }

     pipeline.m_currentVertex += size;

     for (size_t j = 0; j < size - 2; ++j)
     {
       pipeline.m_indices[iOffset + j * 3] = vOffset;
       pipeline.m_indices[iOffset + j * 3 + 1] = vOffset + j + 1;
       pipeline.m_indices[iOffset + j * 3 + 2] = vOffset + j + 2;
     }

     pipeline.m_currentIndex += (size - 2) * 3;
   }

   void GeometryBatcher::addTexturedStrip(
       m2::PointF const * coords,
       m2::PointF const * texCoords,
       unsigned size,
       double depth,
       int pipelineID
       )
   {
     addTexturedStripStrided(coords, sizeof(m2::PointF), texCoords, sizeof(m2::PointF), size, depth, pipelineID);
   }

   void GeometryBatcher::addTexturedStripStrided(
       m2::PointF const * coords,
       size_t coordsStride,
       m2::PointF const * texCoords,
       size_t texCoordsStride,
       unsigned size,
       double depth,
       int pipelineID)
   {
     if (!hasRoom(size, (size - 2) * 3, pipelineID))
       flush(pipelineID);

     GeometryPipeline & pipeline = m_pipelines[pipelineID];

     pipeline.checkStorage(resourceManager());
     if (!pipeline.m_hasStorage)
       return;

     ASSERT(size > 2, ());

     size_t vOffset = pipeline.m_currentVertex;
     size_t iOffset = pipeline.m_currentIndex;

     for (unsigned i = 0; i < size; ++i)
     {
       pipeline.m_vertices[vOffset + i].pt = *coords;
       pipeline.m_vertices[vOffset + i].tex = *texCoords;
       pipeline.m_vertices[vOffset + i].depth = depth;
       coords = reinterpret_cast<m2::PointF const*>(reinterpret_cast<unsigned char const*>(coords) + coordsStride);
       texCoords = reinterpret_cast<m2::PointF const*>(reinterpret_cast<unsigned char const*>(texCoords) + texCoordsStride);
     }

     pipeline.m_currentVertex += size;

     size_t oldIdx1 = vOffset;
     size_t oldIdx2 = vOffset + 1;

     for (size_t j = 0; j < size - 2; ++j)
     {
       pipeline.m_indices[iOffset + j * 3] = oldIdx1;
       pipeline.m_indices[iOffset + j * 3 + 1] = oldIdx2;
       pipeline.m_indices[iOffset + j * 3 + 2] = vOffset + j + 2;

       oldIdx1 = oldIdx2;
       oldIdx2 = vOffset + j + 2;
     }

     pipeline.m_currentIndex += (size - 2) * 3;
   }

   void GeometryBatcher::addTexturedListStrided(
       m2::PointD const * coords,
       size_t coordsStride,
       m2::PointF const * texCoords,
       size_t texCoordsStride,
       unsigned size,
       double depth,
       int pipelineID)
   {
     if (!hasRoom(size, size, pipelineID))
       flush(pipelineID);

     GeometryPipeline & pipeline = m_pipelines[pipelineID];

     pipeline.checkStorage(resourceManager());
     if (!pipeline.m_hasStorage)
       return;

     ASSERT(size > 2, ());

     size_t vOffset = pipeline.m_currentVertex;
     size_t iOffset = pipeline.m_currentIndex;

     for (size_t i = 0; i < size; ++i)
     {
       pipeline.m_vertices[vOffset + i].pt = m2::PointF(coords->x, coords->y);
       pipeline.m_vertices[vOffset + i].tex = *texCoords;
       pipeline.m_vertices[vOffset + i].depth = depth;
       coords = reinterpret_cast<m2::PointD const*>(reinterpret_cast<unsigned char const*>(coords) + coordsStride);
       texCoords = reinterpret_cast<m2::PointF const*>(reinterpret_cast<unsigned char const*>(texCoords) + texCoordsStride);
     }

     pipeline.m_currentVertex += size;

     for (size_t i = 0; i < size; ++i)
       pipeline.m_indices[iOffset + i] = vOffset + i;

     pipeline.m_currentIndex += size;
   }


   void GeometryBatcher::addTexturedListStrided(
       m2::PointF const * coords,
       size_t coordsStride,
       m2::PointF const * texCoords,
       size_t texCoordsStride,
       unsigned size,
       double depth,
       int pipelineID)
   {
     if (!hasRoom(size, size, pipelineID))
       flush(pipelineID);

     GeometryPipeline & pipeline = m_pipelines[pipelineID];

     pipeline.checkStorage(resourceManager());
     if (!pipeline.m_hasStorage)
       return;

     ASSERT(size > 2, ());

     size_t vOffset = pipeline.m_currentVertex;
     size_t iOffset = pipeline.m_currentIndex;

     for (size_t i = 0; i < size; ++i)
     {
       pipeline.m_vertices[vOffset + i].pt = *coords;
       pipeline.m_vertices[vOffset + i].tex = *texCoords;
       pipeline.m_vertices[vOffset + i].depth = depth;
       coords = reinterpret_cast<m2::PointF const*>(reinterpret_cast<unsigned char const*>(coords) + coordsStride);
       texCoords = reinterpret_cast<m2::PointF const*>(reinterpret_cast<unsigned char const*>(texCoords) + texCoordsStride);
     }

     pipeline.m_currentVertex += size;

     for (size_t i = 0; i < size; ++i)
       pipeline.m_indices[iOffset + i] = vOffset + i;

     pipeline.m_currentIndex += size;
   }

   void GeometryBatcher::addTexturedList(m2::PointF const * coords, m2::PointF const * texCoords, unsigned size, double depth, int pipelineID)
   {
     addTexturedListStrided(coords, sizeof(m2::PointF), texCoords, sizeof(m2::PointF), size, depth, pipelineID);
   }

   void GeometryBatcher::enableClipRect(bool flag)
   {
     flush(-1);
     base_t::enableClipRect(flag);
   }

   void GeometryBatcher::setClipRect(m2::RectI const & rect)
   {
     flush(-1);
     base_t::setClipRect(rect);
   }

   int GeometryBatcher::aaShift() const
   {
     return m_aaShift;
   }

   void GeometryBatcher::memoryWarning()
   {
     if (m_skin)
       m_skin->memoryWarning();
   }

   void GeometryBatcher::enterBackground()
   {
     if (m_skin)
       m_skin->enterBackground();
   }

   void GeometryBatcher::enterForeground()
   {
     if (m_skin)
       m_skin->enterForeground();
   }

   void GeometryBatcher::setDisplayList(DisplayList * displayList)
   {
     flush(-1);
     base_t::setDisplayList(displayList);
   }
 } // namespace gl
} // namespace yg

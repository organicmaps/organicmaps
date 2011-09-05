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
    GeometryBatcher::Params::Params() : m_isSynchronized(true)
    {}

    GeometryBatcher::GeometryBatcher(Params const & params)
      : base_t(params), m_isAntiAliased(true), m_isSynchronized(params.m_isSynchronized)
    {
      reset(-1);
      applyStates();

      /// 1 to turn antialiasing on
      /// 2 to switch it off
      m_aaShift = m_isAntiAliased ? 1 : 2;
    }

   void GeometryBatcher::applyStates()
   {
     OGLCHECK(glEnable(GL_TEXTURE_2D));

     if (!m_isAntiAliased)
     {
       OGLCHECK(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST));
       OGLCHECK(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST));
     }

     OGLCHECK(glEnable(GL_DEPTH_TEST));
     OGLCHECK(glDepthFunc(GL_LEQUAL));

     OGLCHECK(glEnable(GL_ALPHA_TEST));
     OGLCHECK(glAlphaFunc(GL_GREATER, 0.0));

     OGLCHECK(glEnable(GL_BLEND));
     OGLCHECK(glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA));

     OGLCHECK(glColor4f(1.0f, 1.0f, 1.0f, 1.0f));
   }

   GeometryBatcher::~GeometryBatcher()
   {}

   void GeometryBatcher::reset(int pageID)
   {
     for (size_t i = 0; i < m_pipelines.size(); ++i)
     {
       if ((pageID == -1) || ((size_t)pageID == i))
       {
         m_pipelines[i].m_currentVertex = 0;
         m_pipelines[i].m_currentIndex = 0;
       }
     }
   }

   void GeometryBatcher::GeometryPipeline::checkStorage(shared_ptr<ResourceManager> const & resourceManager, SkinPage::EUsage usage) const
   {
     if (!m_hasStorage)
     {
       m_storage = usage != SkinPage::EStaticUsage ? resourceManager->storages()->Reserve()
                                                   : resourceManager->smallStorages()->Reserve();

       m_maxVertices = m_storage.m_vertices->size() / sizeof(Vertex);
       m_maxIndices = m_storage.m_indices->size() / sizeof(unsigned short);

       m_vertices = (Vertex*)m_storage.m_vertices->lock();
       m_indices = (unsigned short *)m_storage.m_indices->lock();
       m_hasStorage = true;
     }
   }

   void GeometryBatcher::setSkin(shared_ptr<Skin> skin)
   {
     m_skin = skin;
     if (m_skin != 0)
     {
       m_pipelines.resize(m_skin->pages().size());

       m_skin->addOverflowFn(bind(&GeometryBatcher::flush, this, _1), 100);

       m_skin->addClearPageFn(bind(&GeometryBatcher::flush, this, _1), 100);
       m_skin->addClearPageFn(bind(&GeometryBatcher::switchTextures, this, _1), 99);

       for (size_t i = 0; i < m_pipelines.size(); ++i)
       {
         m_pipelines[i].m_currentVertex = 0;
         m_pipelines[i].m_currentIndex = 0;

         m_pipelines[i].m_hasStorage = false;

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

     if (m_isSynchronized)
       OGLCHECK(glFinish());

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


   bool GeometryBatcher::hasRoom(size_t verticesCount, size_t indicesCount, int pageID) const
   {
     m_pipelines[pageID].checkStorage(resourceManager(), skin()->pages()[pageID]->usage());

     return ((m_pipelines[pageID].m_currentVertex + verticesCount <= m_pipelines[pageID].m_maxVertices)
         &&  (m_pipelines[pageID].m_currentIndex + indicesCount <= m_pipelines[pageID].m_maxIndices));
   }

   size_t GeometryBatcher::verticesLeft(int pageID)
   {
     m_pipelines[pageID].checkStorage(resourceManager(), skin()->pages()[pageID]->usage());

     return m_pipelines[pageID].m_maxVertices - m_pipelines[pageID].m_currentVertex;
   }

   size_t GeometryBatcher::indicesLeft(int pageID)
   {
     m_pipelines[pageID].checkStorage(resourceManager(), skin()->pages()[pageID]->usage());
     return m_pipelines[pageID].m_maxIndices - m_pipelines[pageID].m_currentIndex;
   }

   void GeometryBatcher::flush(int pageID)
   {
     bool renderedData = false;

     if (m_skin)
     {
       for (size_t i = m_pipelines.size(); i > 0; --i)
       {
         if ((pageID == -1) || ((i - 1) == (size_t)pageID))
         {
           shared_ptr<SkinPage> skinPage = m_skin->pages()[i - 1];
           GeometryPipeline & pipeline = m_pipelines[i - 1];

           skinPage->uploadData();

           if (pipeline.m_currentIndex)
           {
             pipeline.m_storage.m_vertices->unlock();
             pipeline.m_storage.m_indices->unlock();

             drawGeometry(skinPage->texture(),
                          pipeline.m_storage.m_vertices,
                          pipeline.m_storage.m_indices,
                          pipeline.m_currentIndex);


             if (isDebugging())
             {
               pipeline.m_verticesDrawn += pipeline.m_currentVertex;
               pipeline.m_indicesDrawn += pipeline.m_currentIndex;
//               LOG(LINFO, ("Pipeline #", i - 1, "draws ", pipeline.m_currentIndex / 3, "/", pipeline.m_maxIndices / 3," triangles"));
             }

             renderedData = true;

             if (skinPage->usage() != SkinPage::EStaticUsage)
               resourceManager()->storages()->Free(pipeline.m_storage);
             else
               resourceManager()->smallStorages()->Free(pipeline.m_storage);

             pipeline.m_hasStorage = false;
             pipeline.m_storage = Storage();
             pipeline.m_maxIndices = 0;
             pipeline.m_maxVertices = 0;
             pipeline.m_vertices = 0;
             pipeline.m_indices = 0;

           }

           reset(i - 1);
         }
       }
     }
   }

   void GeometryBatcher::switchTextures(int pageID)
   {
       m_skin->pages()[pageID]->freeTexture();
//       m_skin->pages()[pageID]->reserveTexture();
   }

   void GeometryBatcher::drawTexturedPolygon(
       m2::PointD const & ptShift,
       ang::AngleD const & angle,
       float tx0, float ty0, float tx1, float ty1,
       float x0, float y0, float x1, float y1,
       double depth,
       int pageID)
   {
     if (!hasRoom(4, 6, pageID))
       flush(pageID);

     m_pipelines[pageID].checkStorage(resourceManager(), skin()->pages()[pageID]->usage());

     float texMinX = tx0;
     float texMaxX = tx1;
     float texMinY = ty0;
     float texMaxY = ty1;

     shared_ptr<BaseTexture> texture = m_skin->pages()[pageID]->texture();

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

     addTexturedFan(coords, texCoords, 4, depth, pageID);
   }

   void GeometryBatcher::addTexturedFan(m2::PointF const * coords, m2::PointF const * texCoords, unsigned size, double depth, int pageID)
   {
     if (!hasRoom(size, (size - 2) * 3, pageID))
       flush(pageID);

     m_pipelines[pageID].checkStorage(resourceManager(), skin()->pages()[pageID]->usage());

     ASSERT(size > 2, ());

     size_t vOffset = m_pipelines[pageID].m_currentVertex;
     size_t iOffset = m_pipelines[pageID].m_currentIndex;

     for (unsigned i = 0; i < size; ++i)
     {
       m_pipelines[pageID].m_vertices[vOffset + i].pt = coords[i];
       m_pipelines[pageID].m_vertices[vOffset + i].tex = texCoords[i];
       m_pipelines[pageID].m_vertices[vOffset + i].depth = depth;
     }


     m_pipelines[pageID].m_currentVertex += size;

     for (size_t j = 0; j < size - 2; ++j)
     {
       m_pipelines[pageID].m_indices[iOffset + j * 3] = vOffset;
       m_pipelines[pageID].m_indices[iOffset + j * 3 + 1] = vOffset + j + 1;
       m_pipelines[pageID].m_indices[iOffset + j * 3 + 2] = vOffset + j + 2;
     }

     m_pipelines[pageID].m_currentIndex += (size - 2) * 3;
   }

   void GeometryBatcher::addTexturedStrip(
       m2::PointF const * coords,
       m2::PointF const * texCoords,
       unsigned size,
       double depth,
       int pageID
       )
   {
     addTexturedStripStrided(coords, sizeof(m2::PointF), texCoords, sizeof(m2::PointF), size, depth, pageID);
   }

   void GeometryBatcher::addTexturedStripStrided(
       m2::PointF const * coords,
       size_t coordsStride,
       m2::PointF const * texCoords,
       size_t texCoordsStride,
       unsigned size,
       double depth,
       int pageID)
   {
     if (!hasRoom(size, (size - 2) * 3, pageID))
       flush(pageID);

     m_pipelines[pageID].checkStorage(resourceManager(), skin()->pages()[pageID]->usage());

     ASSERT(size > 2, ());

     size_t vOffset = m_pipelines[pageID].m_currentVertex;
     size_t iOffset = m_pipelines[pageID].m_currentIndex;

     for (unsigned i = 0; i < size; ++i)
     {
       m_pipelines[pageID].m_vertices[vOffset + i].pt = *coords;
       m_pipelines[pageID].m_vertices[vOffset + i].tex = *texCoords;
       m_pipelines[pageID].m_vertices[vOffset + i].depth = depth;
       coords = reinterpret_cast<m2::PointF const*>(reinterpret_cast<unsigned char const*>(coords) + coordsStride);
       texCoords = reinterpret_cast<m2::PointF const*>(reinterpret_cast<unsigned char const*>(texCoords) + texCoordsStride);
     }

     m_pipelines[pageID].m_currentVertex += size;

     size_t oldIdx1 = vOffset;
     size_t oldIdx2 = vOffset + 1;

     for (size_t j = 0; j < size - 2; ++j)
     {
       m_pipelines[pageID].m_indices[iOffset + j * 3] = oldIdx1;
       m_pipelines[pageID].m_indices[iOffset + j * 3 + 1] = oldIdx2;
       m_pipelines[pageID].m_indices[iOffset + j * 3 + 2] = vOffset + j + 2;

       oldIdx1 = oldIdx2;
       oldIdx2 = vOffset + j + 2;
     }

     m_pipelines[pageID].m_currentIndex += (size - 2) * 3;
   }

   void GeometryBatcher::addTexturedListStrided(
       m2::PointD const * coords,
       size_t coordsStride,
       m2::PointF const * texCoords,
       size_t texCoordsStride,
       unsigned size,
       double depth,
       int pageID)
   {
     if (!hasRoom(size, size, pageID))
       flush(pageID);

     m_pipelines[pageID].checkStorage(resourceManager(), skin()->pages()[pageID]->usage());

     ASSERT(size > 2, ());

     size_t vOffset = m_pipelines[pageID].m_currentVertex;
     size_t iOffset = m_pipelines[pageID].m_currentIndex;

     for (size_t i = 0; i < size; ++i)
     {
       m_pipelines[pageID].m_vertices[vOffset + i].pt = m2::PointF(coords->x, coords->y);
       m_pipelines[pageID].m_vertices[vOffset + i].tex = *texCoords;
       m_pipelines[pageID].m_vertices[vOffset + i].depth = depth;
       coords = reinterpret_cast<m2::PointD const*>(reinterpret_cast<unsigned char const*>(coords) + coordsStride);
       texCoords = reinterpret_cast<m2::PointF const*>(reinterpret_cast<unsigned char const*>(texCoords) + texCoordsStride);
     }

     m_pipelines[pageID].m_currentVertex += size;

     for (size_t i = 0; i < size; ++i)
       m_pipelines[pageID].m_indices[iOffset + i] = vOffset + i;

     m_pipelines[pageID].m_currentIndex += size;
   }


   void GeometryBatcher::addTexturedListStrided(
       m2::PointF const * coords,
       size_t coordsStride,
       m2::PointF const * texCoords,
       size_t texCoordsStride,
       unsigned size,
       double depth,
       int pageID)
   {
     if (!hasRoom(size, size, pageID))
       flush(pageID);

     m_pipelines[pageID].checkStorage(resourceManager(), skin()->pages()[pageID]->usage());

     ASSERT(size > 2, ());

     size_t vOffset = m_pipelines[pageID].m_currentVertex;
     size_t iOffset = m_pipelines[pageID].m_currentIndex;

     for (size_t i = 0; i < size; ++i)
     {
       m_pipelines[pageID].m_vertices[vOffset + i].pt = *coords;
       m_pipelines[pageID].m_vertices[vOffset + i].tex = *texCoords;
       m_pipelines[pageID].m_vertices[vOffset + i].depth = depth;
       coords = reinterpret_cast<m2::PointF const*>(reinterpret_cast<unsigned char const*>(coords) + coordsStride);
       texCoords = reinterpret_cast<m2::PointF const*>(reinterpret_cast<unsigned char const*>(texCoords) + texCoordsStride);
     }

     m_pipelines[pageID].m_currentVertex += size;

     for (size_t i = 0; i < size; ++i)
       m_pipelines[pageID].m_indices[iOffset + i] = vOffset + i;

     m_pipelines[pageID].m_currentIndex += size;
   }

   void GeometryBatcher::addTexturedList(m2::PointF const * coords, m2::PointF const * texCoords, unsigned size, double depth, int pageID)
   {
     addTexturedListStrided(coords, sizeof(m2::PointF), texCoords, sizeof(m2::PointF), size, depth, pageID);
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
 } // namespace gl
} // namespace yg

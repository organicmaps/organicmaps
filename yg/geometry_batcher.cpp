#include "../base/SRC_FIRST.hpp"

#include "geometry_batcher.hpp"
#include "skin.hpp"
#include "memento.hpp"
#include "color.hpp"
#include "utils.hpp"
#include "resource_manager.hpp"

#include "internal/opengl.hpp"

#include "../coding/strutil.hpp"

#include "../geometry/rect2d.hpp"

#include "../base/assert.hpp"
#include "../base/profiler.hpp"
#include "../base/math.hpp"
#include "../base/mutex.hpp"
#include "../base/logging.hpp"

#include "../std/algorithm.hpp"
#include "../std/bind.hpp"

#include "../base/start_mem_debug.hpp"

namespace yg
{
  namespace gl
  {
    GeometryBatcher::GeometryBatcher(Params const & params)
      : base_t(params), m_isAntiAliased(!params.m_isMultiSampled)
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

     OGLCHECK(glEnable(GL_DEPTH_TEST));
     OGLCHECK(glDepthFunc(GL_LEQUAL));

     OGLCHECK(glEnable(GL_ALPHA_TEST));
     OGLCHECK(glAlphaFunc(GL_GREATER, 0));

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

         m_pipelines[i].m_storage = m_skin->pages()[i]->isDynamic() ? resourceManager()->reserveStorage() : resourceManager()->reserveSmallStorage();

         m_pipelines[i].m_maxVertices = m_pipelines[i].m_storage.m_vertices->size() / sizeof(Vertex);
         m_pipelines[i].m_maxIndices = m_pipelines[i].m_storage.m_indices->size() / sizeof(unsigned short);

         m_pipelines[i].m_vertices = (Vertex*)m_pipelines[i].m_storage.m_vertices->lock();
         m_pipelines[i].m_indices = (unsigned short *)m_pipelines[i].m_storage.m_indices->lock();
       }
     }
   }

   shared_ptr<Skin> GeometryBatcher::skin() const
   {
     return m_skin;
   }

   void GeometryBatcher::beginFrame()
   {
     base_t::beginFrame();
     reset(-1);
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
     OGLCHECK(glFinish());
     base_t::endFrame();
   }


   bool GeometryBatcher::hasRoom(size_t verticesCount, size_t indicesCount, int pageID) const
   {
     return ((m_pipelines[pageID].m_currentVertex + verticesCount <= m_pipelines[pageID].m_maxVertices)
         &&  (m_pipelines[pageID].m_currentIndex + indicesCount <= m_pipelines[pageID].m_maxIndices));
   }

   size_t GeometryBatcher::verticesLeft(int pageID)
   {
     return m_pipelines[pageID].m_maxVertices - m_pipelines[pageID].m_currentVertex;
   }

   size_t GeometryBatcher::indicesLeft(int pageID)
   {
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


//             LOG(LINFO, ("Pipeline #", i - 1, "draws ", pipeline.m_currentIndex / 3, " triangles"));

             renderedData = true;

             if (skinPage->isDynamic())
               resourceManager()->freeStorage(pipeline.m_storage);
             else
               resourceManager()->freeSmallStorage(pipeline.m_storage);
             pipeline.m_storage = skinPage->isDynamic() ? resourceManager()->reserveStorage() : resourceManager()->reserveSmallStorage();
             pipeline.m_maxVertices = pipeline.m_storage.m_vertices->size() / sizeof(Vertex);
             pipeline.m_maxIndices = pipeline.m_storage.m_indices->size() / sizeof(unsigned short);

             pipeline.m_vertices = (Vertex*)pipeline.m_storage.m_vertices->lock();
             pipeline.m_indices = (unsigned short*)pipeline.m_storage.m_indices->lock();
           }

           reset(i - 1);
         }
       }
     }
   }

   void GeometryBatcher::switchTextures(int pageID)
   {
//     if (m_pipelines[pageID].m_currentIndex > 0)
//     {
//       LOG(LINFO, ("Improving Parallelism ;)"));
       m_skin->pages()[pageID]->freeTexture();
       m_skin->pages()[pageID]->reserveTexture();
//     }
   }



   void GeometryBatcher::drawTrianglesList(m2::PointD const * points, size_t pointsCount, uint32_t styleID, double depth)
   {
     ResourceStyle const * style = m_skin->fromID(styleID);

     if (style == 0)
     {
       LOG(LINFO, ("styleID=", styleID, " wasn't found on current skin."));
       return;
     }

     if (!hasRoom(pointsCount, pointsCount, style->m_pageID))
       flush(style->m_pageID);

     ASSERT_GREATER_OR_EQUAL(pointsCount, 2, ());

     float texX = style->m_texRect.minX() + 1.0f;
     float texY = style->m_texRect.minY() + 1.0f;

     m_skin->pages()[style->m_pageID]->texture()->mapPixel(texX, texY);

     size_t pointsLeft = pointsCount;
     size_t batchOffset = 0;

     while (true)
     {
       size_t batchSize = pointsLeft;

       if (batchSize > verticesLeft(style->m_pageID))
         /// Rounding to the boundary of 3 vertices
         batchSize = verticesLeft(style->m_pageID) / 3 * 3;

       if (batchSize > indicesLeft(style->m_pageID))
         batchSize = indicesLeft(style->m_pageID) / 3 * 3;

       bool needToFlush = (batchSize < pointsLeft);

       int vOffset = m_pipelines[style->m_pageID].m_currentVertex;
       int iOffset = m_pipelines[style->m_pageID].m_currentIndex;

       for (size_t i = 0; i < batchSize; ++i)
       {
         m_pipelines[style->m_pageID].m_vertices[vOffset + i].pt = points[batchOffset + i];
         m_pipelines[style->m_pageID].m_vertices[vOffset + i].tex = m2::PointF(texX, texY);
         m_pipelines[style->m_pageID].m_vertices[vOffset + i].depth = depth;
       }

       for (size_t i = 0; i < batchSize; ++i)
         m_pipelines[style->m_pageID].m_indices[iOffset + i] = vOffset + i;

       batchOffset += batchSize;

       m_pipelines[style->m_pageID].m_currentVertex += batchSize;
       m_pipelines[style->m_pageID].m_currentIndex += batchSize;

       pointsLeft -= batchSize;

       if (needToFlush)
         flush(style->m_pageID);

       if (pointsLeft == 0)
         break;
     }
   }

   void GeometryBatcher::drawTexturedPolygon(
       m2::PointD const & ptShift,
       float angle,
       float tx0, float ty0, float tx1, float ty1,
       float x0, float y0, float x1, float y1,
       double depth,
       int pageID)
   {
     if (!hasRoom(4, 6, pageID))
       flush(pageID);

     float texMinX = tx0;
     float texMaxX = tx1;
     float texMinY = ty0;
     float texMaxY = ty1;

     shared_ptr<BaseTexture> texture = m_skin->pages()[pageID]->texture();

     texture->mapPixel(texMinX, texMinY);
     texture->mapPixel(texMaxX, texMaxY);

     // vng: enough to calc it once
     double const sinA = sin(angle);
     double const cosA = cos(angle);

     /// rotated and translated four points (x0, y0), (x0, y1), (x1, y1), (x1, y0)

     m2::PointF coords[4] =
     {
       m2::PointF(x0 * cosA - y0 * sinA + ptShift.x, x0 * sinA + y0 * cosA + ptShift.y),
       m2::PointF(x0 * cosA - y1 * sinA + ptShift.x, x0 * sinA + y1 * cosA + ptShift.y),
       m2::PointF(x1 * cosA - y1 * sinA + ptShift.x, x1 * sinA + y1 * cosA + ptShift.y),
       m2::PointF(x1 * cosA - y0 * sinA + ptShift.x, x1 * sinA + y0 * cosA + ptShift.y)
     };

     /// Special case. Making straight fonts sharp.
     if (angle == 0)
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

   void GeometryBatcher::addTexturedStrip(m2::PointF const * coords, m2::PointF const * texCoords, unsigned size, double depth, int pageID)
   {
     if (!hasRoom(size, (size - 2) * 3, pageID))
       flush(pageID);

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


 } // namespace gl
} // namespace yg

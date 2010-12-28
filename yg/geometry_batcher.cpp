#include "../base/SRC_FIRST.hpp"

#include "geometry_batcher.hpp"
#include "skin.hpp"
#include "memento.hpp"
#include "color.hpp"
#include "utils.hpp"
#include "resource_manager.hpp"

#include "internal/opengl.hpp"

#include "../coding/strutil.hpp"

#include "../geometry/angles.hpp"

#include "../base/assert.hpp"
#include "../base/profiler.hpp"
#include "../base/math.hpp"
#include "../base/mutex.hpp"
#include "../base/logging.hpp"
#include "../geometry/rect2d.hpp"
#include "../std/algorithm.hpp"
#include "../std/bind.hpp"

#include "../base/start_mem_debug.hpp"

namespace yg
{
  namespace gl
  {
    GeometryBatcher::GeometryBatcher(Params const & params)
      : base_t(params), m_isAntiAliased(params.m_isAntiAliased)
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

   void GeometryBatcher::drawPoint(m2::PointD const & pt, uint32_t styleID, double depth)
   {
     ResourceStyle const * style(m_skin->fromID(styleID));

     if (!hasRoom(4, 6, style->m_pageID))
       flush(style->m_pageID);

     m2::RectU texRect(style->m_texRect.minX() + 1,
                       style->m_texRect.minY() + 1,
                       style->m_texRect.maxX() - 1,
                       style->m_texRect.maxY() - 1);

     float polyMinX = my::rounds(pt.x - (style->m_texRect.SizeX() - 2) / 2.0);
     float polyMaxX = polyMinX + (style->m_texRect.SizeX() - 2);

     float polyMinY = my::rounds(pt.y - (style->m_texRect.SizeY() - 2) / 2.0);
     float polyMaxY = polyMinY + (style->m_texRect.SizeY() - 2);

     drawTexturedPolygon(m2::PointU(0, 0), 0,
                         texRect.minX(), texRect.minY(), texRect.maxX(), texRect.maxY(),
                         polyMinX, polyMinY, polyMaxX, polyMaxY,
                         depth,
                         style->m_pageID);
   }

   void GeometryBatcher::drawPath(m2::PointD const * points, size_t pointsCount, uint32_t styleID, double depth)
   {
#ifdef PROFILER_YG
     prof::block<prof::yg_draw_path> draw_path_block;
#endif

//#ifdef DEBUG_DRAW_PATH
//     LineStyle const * lineStyle = reinterpret_cast<LineStyle const*>(styleID);
//#else

     ASSERT_GREATER_OR_EQUAL(pointsCount, 2, ());
     ResourceStyle const * style(m_skin->fromID(styleID));

     ASSERT(style->m_cat == ResourceStyle::ELineStyle, ());
     LineStyle const * lineStyle = static_cast<LineStyle const *>(style);
//#endif

     float rawTileStartLen = 0;

     for (size_t i = 0; i < pointsCount - 1; ++i)
     {
       m2::PointD dir = points[i + 1] - points[i];
       dir *= 1.0 / dir.Length(m2::PointD(0, 0));
       m2::PointD norm(-dir.y, dir.x);

       /// The length of the current segment.
       float segLen = points[i + 1].Length(points[i]);
       /// The remaining length of the segment
       float segLenRemain = segLen;

       /// Geometry width. It's 1px wider than the pattern width.
       int geomWidth = lineStyle->m_isSolid ? lineStyle->m_penInfo.m_w : lineStyle->m_penInfo.m_w + 4 - 2 * m_aaShift;
       float geomHalfWidth =  geomWidth / 2.0;

       /// Starting point of the tiles on this segment
       m2::PointF rawTileStartPt = points[i];

       /// Tiling procedes as following :
       /// The leftmost tile goes antialiased at left and non-antialiased at right.
       /// The inner tiles goes non-antialiased at both sides.
       /// The rightmost tile goes non-antialised at left and antialiased at right side.

       /// Length of the actual pattern data being tiling(without antialiasing zones).
       float rawTileLen = 0;
       while (segLenRemain > 0)
       {
         rawTileLen = (lineStyle->m_isWrapped || lineStyle->m_isSolid)
                      ? segLen
                      : std::min(((float)lineStyle->rawTileLen() - rawTileStartLen), segLenRemain);

         float texMaxY = lineStyle->m_isSolid ? lineStyle->m_texRect.minY() + 1 : lineStyle->m_texRect.maxY() - m_aaShift;
         float texMinY = lineStyle->m_isSolid ? lineStyle->m_texRect.minY() + 1 : lineStyle->m_texRect.minY() + m_aaShift;

         float texMinX = lineStyle->m_isSolid ? lineStyle->m_texRect.minX() + 1 : lineStyle->m_isWrapped ? 0 : lineStyle->m_texRect.minX() + 2 + rawTileStartLen;
         float texMaxX = lineStyle->m_isSolid ? lineStyle->m_texRect.minX() + 1 : texMinX + rawTileLen;

         rawTileStartLen += rawTileLen;
         if (rawTileStartLen >= lineStyle->rawTileLen())
           rawTileStartLen -= lineStyle->rawTileLen();
         ASSERT(rawTileStartLen < lineStyle->rawTileLen(), ());

         m2::PointF rawTileEndPt(rawTileStartPt.x + dir.x * rawTileLen, rawTileStartPt.y + dir.y * rawTileLen);

         m2::PointF const fNorm = norm * geomHalfWidth;  // enough to calc it once
         m2::PointF coords[4] =
         {
           // vng: i think this "rawTileStartPt + fNorm" reading better, isn't it?
           m2::PointF(rawTileStartPt.x + fNorm.x, rawTileStartPt.y + fNorm.y),
           m2::PointF(rawTileStartPt.x - fNorm.x, rawTileStartPt.y - fNorm.y),
           m2::PointF(rawTileEndPt.x - fNorm.x, rawTileEndPt.y - fNorm.y),
           m2::PointF(rawTileEndPt.x + fNorm.x, rawTileEndPt.y + fNorm.y)
         };

         shared_ptr<BaseTexture> texture = m_skin->pages()[lineStyle->m_pageID]->texture();

         m2::PointF texCoords[4] =
         {
           texture->mapPixel(m2::PointF(texMinX, texMinY)),
           texture->mapPixel(m2::PointF(texMinX, texMaxY)),
           texture->mapPixel(m2::PointF(texMaxX, texMaxY)),
           texture->mapPixel(m2::PointF(texMaxX, texMinY))
         };

         addTexturedVertices(coords, texCoords, 4, depth, lineStyle->m_pageID);

         segLenRemain -= rawTileLen;

         rawTileStartPt = rawTileEndPt;
       }

       bool isColorJoin = lineStyle->m_isSolid ? true : lineStyle->m_penInfo.atDashOffset(rawTileLen);

       /// Adding geometry for a line join between previous and current segment.
       if ((i != pointsCount - 2) && (isColorJoin))
       {
         m2::PointD nextDir = points[i + 2] - points[i + 1];
         nextDir *= 1.0 / nextDir.Length(m2::PointD(0, 0));
         m2::PointD nextNorm(-nextDir.y, nextDir.x);

         /// Computing the sin of angle between directions.
         double alphaSin = dir.x * nextDir.y - dir.y * nextDir.x;
         double alphaCos = dir.x * nextDir.x + dir.y * nextDir.y;
         double alpha = atan2(alphaSin, alphaCos);
         int angleSegCount = int(ceil(fabs(alpha) / (math::pi / 6)));
         double angleStep = alpha / angleSegCount;

         m2::PointD startVec;

         if (alpha > 0)
         {
           /// The outer site is on the prevNorm direction.
           startVec = -norm;
         }
         else
         {
           /// The outer site is on the -prevNorm direction
           startVec = norm;
         }

         shared_ptr<BaseTexture> texture = m_skin->pages()[lineStyle->m_pageID]->texture();

         m2::PointF joinSegTex[3] =
         {
           texture->mapPixel(lineStyle->m_centerColorPixel),
           texture->mapPixel(lineStyle->m_borderColorPixel),
           texture->mapPixel(lineStyle->m_borderColorPixel)
         };

         m2::PointD prevStartVec = startVec;
         for (int j = 0; j < angleSegCount; ++j)
         {
           /// Rotate start vector to find another point on a join.
           startVec.Rotate(angleStep);

           /// Computing three points of a join segment.
           m2::PointF joinSeg[3] =
           {
             m2::PointF(points[i + 1]),
             m2::PointF(points[i + 1] + startVec * geomHalfWidth),
             m2::PointF(points[i + 1] + prevStartVec * geomHalfWidth)
           };

           addTexturedVertices(joinSeg, joinSegTex, 3, depth, lineStyle->m_pageID);

           prevStartVec = startVec;
         }
       }
     }
   }

   void GeometryBatcher::drawTriangles(m2::PointD const * points, size_t pointsCount, uint32_t styleID, double depth)
   {
     ResourceStyle const * style = m_skin->fromID(styleID);
     if (!hasRoom(pointsCount, (pointsCount - 2) * 3, style->m_pageID))
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

     addTexturedVertices(coords, texCoords, 4, depth, pageID);
   }

   void GeometryBatcher::addTexturedVertices(m2::PointF const * coords, m2::PointF const * texCoords, unsigned size, double depth, int pageID)
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

   void GeometryBatcher::drawGlyph(m2::PointD const & ptOrg, m2::PointD const & ptGlyph, float angle, float blOffset, CharStyle const * p, double depth)
   {
     float x0 = ptGlyph.x + (p->m_xOffset - 1);
     float y1 = ptGlyph.y - (p->m_yOffset - 1) - blOffset;
     float y0 = y1 - (p->m_texRect.SizeY() - 2);
     float x1 = x0 + (p->m_texRect.SizeX() - 2);


     drawTexturedPolygon(ptOrg, angle,
                         p->m_texRect.minX() + 1,
                         p->m_texRect.minY() + 1,
                         p->m_texRect.maxX() - 1,
                         p->m_texRect.maxY() - 1,
                         x0, y0, x1, y1,
                         depth,
                         p->m_pageID);
   }

   template <class ToDo>
   void GeometryBatcher::ForEachGlyph(uint8_t fontSize, wstring const & text, bool isMask, ToDo toDo)
   {
     m2::PointD currPt(0, 0);
     for (size_t i = 0; i < text.size(); ++i)
     {
       uint32_t glyphID = m_skin->mapGlyph(GlyphKey(text[i], fontSize, isMask));
       CharStyle const * p = static_cast<CharStyle const *>(m_skin->fromID(glyphID));
       if (p)
       {
         toDo(currPt, p);
         currPt += m2::PointD(p->m_xAdvance, 0);
       }
     }
   }

   void GeometryBatcher::drawText(m2::PointD const & pt, float angle, uint8_t fontSize, string const & utf8Text, double depth)
   {
     wstring text = FromUtf8(utf8Text);

     ForEachGlyph(fontSize, text, true, bind(&GeometryBatcher::drawGlyph, this, cref(pt), _1, angle, 0, _2, depth));
     ForEachGlyph(fontSize, text, false, bind(&GeometryBatcher::drawGlyph, this, cref(pt), _1, angle, 0, _2, depth));
   }

   /// Incapsulate array of points in readable getting direction.
   class pts_array
   {
     m2::PointD const * m_arr;
     size_t m_size;
     bool m_reverse;

   public:
     pts_array(m2::PointD const * arr, size_t sz)
       : m_arr(arr), m_size(sz), m_reverse(false)
     {
       ASSERT ( m_size > 1, () );

       /* assume, that readable text in path should be ('o' - start draw point):
        *    /   o
        *   /     \
        *  /   or  \
        * o         \
       */
       double const a = ang::AngleTo(get(0), get(1));
       if (fabs(a) > math::pi / 2.0)
         m_reverse = true;
     }

     size_t size() const { return m_size; }

     m2::PointD get(size_t i) const
     {
       ASSERT ( i < m_size, ("Index out of range") );
       return m_arr[m_reverse ? m_size - i - 1 : i];
     }
     m2::PointD operator[](size_t i) const { return get(i); }
   };

   double const angle_not_inited = -100.0;

   bool CalcPointAndAngle(pts_array const & arr, double offset, size_t & ind, m2::PointD & pt, double & angle)
   {
     size_t const oldInd = ind;

     while (true)
     {
       if (ind + 1 == arr.size())
         return false;

       double const l = arr[ind + 1].Length(pt);
       if (offset < l)
         break;

       offset -= l;
       pt = arr[++ind];
     }

     if (oldInd != ind || angle == angle_not_inited)
       angle = ang::AngleTo(pt, arr[ind + 1]);
     pt = pt.Move(offset, angle);
     return true;
   }

   void GeometryBatcher::drawPathText(m2::PointD const * path, size_t s, uint8_t fontSize, string const & utf8Text,
                             double pathLength, TextPos pos, bool isMasked, double depth)
   {
     if (isMasked)
       drawPathTextImpl(path, s, fontSize, utf8Text, pathLength, pos, true, depth);
     drawPathTextImpl(path, s, fontSize, utf8Text, pathLength, pos, false, depth);
   }

   void GeometryBatcher::drawPathTextImpl(m2::PointD const * path, size_t s, uint8_t fontSize, string const & utf8Text,
                             double pathLength, TextPos pos, bool fromMask, double depth)
   {
     pts_array arrPath(path, s);

     wstring const text = FromUtf8(utf8Text);

//     FontInfo const & fontInfo = fontSize > 0 ? m_skin->getBigFont() : m_skin->getSmallFont();
//     FontInfo const & fontInfo = m_skin->getFont(translateFontSize(fontSize));

//     fontSize = translateFontSize(fontSize);

     // calc base line offset
     float blOffset = 2;
     switch (pos)
     {
     case under_line: blOffset -= fontSize; break;
     case middle_line: blOffset -= fontSize / 2; break;
     case above_line: blOffset -= 0; break;
     }

     // calc string length
     double strLength = 0.0;
     for (size_t i = 0; i < text.size(); ++i)
     {
       uint32_t glyphID = m_skin->mapGlyph(GlyphKey(text[i], fontSize, fromMask));
       CharStyle const * p = static_cast<CharStyle const *>(m_skin->fromID(glyphID));
       strLength += p->m_xAdvance;
     }

     // offset of the text fromt path's start
     double offset = (pathLength - strLength) / 2.0;
     if (offset < 0.0) return;

     size_t ind = 0;
     m2::PointD ptOrg = arrPath[0];
     double angle = angle_not_inited;

     for (size_t i = 0; i < text.size(); ++i)
     {
       if (!CalcPointAndAngle(arrPath, offset, ind, ptOrg, angle))
         break;

       uint32_t glyphID = m_skin->mapGlyph(GlyphKey(text[i], fontSize, fromMask));
       CharStyle const * p = static_cast<CharStyle const *>(m_skin->fromID(glyphID));

       drawGlyph(ptOrg, m2::PointD(0.0, 0.0), angle, blOffset, p, depth);

       offset = p->m_xAdvance;
     }
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

 } // namespace gl
} // namespace yg

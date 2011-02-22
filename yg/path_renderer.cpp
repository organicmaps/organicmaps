#include "../base/SRC_FIRST.hpp"

#include "path_renderer.hpp"
#include "resource_style.hpp"
#include "skin.hpp"

#include "../base/logging.hpp"

namespace yg
{
  namespace gl
  {
    PathRenderer::Params::Params() : m_drawPathes(true)
    {}

    PathRenderer::PathRenderer(Params const & params)
      : base_t(params), m_drawPathes(params.m_drawPathes)
    {}

    void PathRenderer::drawPath(m2::PointD const * points, size_t pointsCount, uint32_t styleID, double depth)
    {
      ++m_pathCount;
      m_pointsCount += pointsCount;

      if (!m_drawPathes)
        return;

 #ifdef PROFILER_YG
      prof::block<prof::yg_draw_path> draw_path_block;
 #endif

 //#ifdef DEBUG_DRAW_PATH
 //     LineStyle const * lineStyle = reinterpret_cast<LineStyle const*>(styleID);
 //#else

      ASSERT_GREATER_OR_EQUAL(pointsCount, 2, ());
      ResourceStyle const * style(skin()->fromID(styleID));

      if (style == 0)
      {
        LOG(LINFO, ("styleID=", styleID, " wasn't found on current skin"));
        return;
      }

      ASSERT(style->m_cat == ResourceStyle::ELineStyle, ());
      LineStyle const * lineStyle = static_cast<LineStyle const *>(style);
 //#endif

      if (lineStyle->m_isSolid)
      {
        drawSolidPath(points, pointsCount, styleID, depth);
        return;
      }

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
        int geomWidth = lineStyle->m_isSolid ? lineStyle->m_penInfo.m_w : lineStyle->m_penInfo.m_w + 4 - 2 * aaShift();
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

          float texMaxY = lineStyle->m_isSolid ? lineStyle->m_texRect.minY() + 1 : lineStyle->m_texRect.maxY() - aaShift();
          float texMinY = lineStyle->m_isSolid ? lineStyle->m_texRect.minY() + 1 : lineStyle->m_texRect.minY() + aaShift();

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

          shared_ptr<BaseTexture> texture = skin()->pages()[lineStyle->m_pageID]->texture();

          m2::PointF texCoords[4] =
          {
            texture->mapPixel(m2::PointF(texMinX, texMinY)),
            texture->mapPixel(m2::PointF(texMinX, texMaxY)),
            texture->mapPixel(m2::PointF(texMaxX, texMaxY)),
            texture->mapPixel(m2::PointF(texMaxX, texMinY))
          };

          addTexturedFan(coords, texCoords, 4, depth, lineStyle->m_pageID);

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

          shared_ptr<BaseTexture> texture = skin()->pages()[lineStyle->m_pageID]->texture();

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

            addTexturedFan(joinSeg, joinSegTex, 3, depth, lineStyle->m_pageID);

            prevStartVec = startVec;
          }
        }
      }
    }

    void PathRenderer::drawSolidPath(m2::PointD const * points, size_t pointsCount, uint32_t styleID, double depth)
    {
      ASSERT_GREATER_OR_EQUAL(pointsCount, 2, ());
      ResourceStyle const * style(skin()->fromID(styleID));

      ASSERT(style->m_cat == ResourceStyle::ELineStyle, ());
      LineStyle const * lineStyle = static_cast<LineStyle const *>(style);

      ASSERT(lineStyle->m_isSolid, ());

      for (size_t i = 0; i < pointsCount - 1; ++i)
      {
        m2::PointD dir = points[i + 1] - points[i];
        double len = dir.Length(m2::PointD(0, 0));
        dir *= 1.0 / len;
        m2::PointD norm(-dir.y, dir.x);
        m2::PointD const & nextPt = points[i + 1];

        float geomHalfWidth = (lineStyle->m_penInfo.m_w + 4 - aaShift() * 2) / 2.0;

        float texMinX = lineStyle->m_texRect.minX() + 1;
        float texMaxX = lineStyle->m_texRect.maxX() - 1;

        float texMinY = lineStyle->m_texRect.maxY() - aaShift();
        float texMaxY = lineStyle->m_texRect.minY() + aaShift();

        float texCenterX = (texMinX + texMaxX) / 2;

        m2::PointF const fNorm = norm * geomHalfWidth;  // enough to calc it once
        m2::PointF const fDir(fNorm.y, -fNorm.x);

        m2::PointF coords[8] =
        {
          /// left round cap
          points[i] - fDir + fNorm,
          points[i] - fDir - fNorm,
          points[i] + fNorm,
          /// inner part
          points[i] - fNorm,
          nextPt + fNorm,
          nextPt - fNorm,
          /// right round cap
          nextPt + fDir + fNorm,
          nextPt + fDir - fNorm
        };

        shared_ptr<BaseTexture> texture = skin()->pages()[lineStyle->m_pageID]->texture();

        m2::PointF texCoords[8] =
        {
          texture->mapPixel(m2::PointF(texMinX, texMinY)),
          texture->mapPixel(m2::PointF(texMinX, texMaxY)),
          texture->mapPixel(m2::PointF(texCenterX, texMinY)),
          texture->mapPixel(m2::PointF(texCenterX, texMaxY)),
          texture->mapPixel(m2::PointF(texCenterX, texMinY)),
          texture->mapPixel(m2::PointF(texCenterX, texMaxY)),
          texture->mapPixel(m2::PointF(texMaxX, texMinY)),
          texture->mapPixel(m2::PointF(texMaxX, texMaxY))
        };

        addTexturedStrip(coords, texCoords, 8, depth, lineStyle->m_pageID);
      }
    }

    void PathRenderer::beginFrame()
    {
      base_t::beginFrame();
      m_pathCount = 0;
      m_pointsCount = 0;
    }

    void PathRenderer::endFrame()
    {
      if (isDebugging())
        LOG(LINFO, ("Drawing ", m_pathCount, " pathes, ", m_pointsCount, " points total"));

      base_t::endFrame();
    }
  }
}


#include "path_renderer.hpp"
#include "resource.hpp"
#include "pen.hpp"
#include "resource_cache.hpp"

#include "opengl/base_texture.hpp"

#include "../base/logging.hpp"

namespace graphics
{
  PathRenderer::Params::Params()
    : m_drawPathes(true),
      m_fastSolidPath(true)
  {}

  PathRenderer::PathRenderer(Params const & params)
    : base_t(params),
      m_drawPathes(params.m_drawPathes),
      m_fastSolidPath(params.m_fastSolidPath)
  {}

  void PathRenderer::drawPath(m2::PointD const * pts, size_t ptsCount, double offset, uint32_t resID, double depth)
  {
    ++m_pathCount;
    m_pointsCount += ptsCount;

    if (!m_drawPathes)
      return;

    ASSERT_GREATER_OR_EQUAL(ptsCount, 2, ());
    ASSERT_NOT_EQUAL(resID, uint32_t(-1), ());

    Resource const * res = base_t::fromID(resID);

    if (res == 0)
    {
      LOG(LINFO, ("drawPath: resID=", resID, "wasn't found on current skin"));
      return;
    }

    ASSERT(res->m_cat == Resource::EPen, ());

    Pen const * pen = static_cast<Pen const *>(res);

    if (!pen->m_info.m_symbol.empty())
      drawSymbolPath(pts, ptsCount, offset, pen, depth);
    else
      if (m_fastSolidPath && pen->m_isSolid)
        drawSolidPath(pts, ptsCount, offset, pen, depth);
      else
        drawStipplePath(pts, ptsCount, offset, pen, depth);
  }

  void PathRenderer::drawSymbolPath(m2::PointD const * pts, size_t ptsCount, double offset, Pen const * pen, double depth)
  {
    LOG(LINFO, ("drawSymbolPath is unimplemented. symbolName=", pen->m_info.m_symbol));
  }

  void PathRenderer::drawStipplePath(m2::PointD const * points, size_t pointsCount, double offset, Pen const * pen, double depth)
  {
    float rawTileStartLen = 0;

    float rawTileLen = (float)pen->rawTileLen();

    if ((offset < 0) && (!pen->m_isWrapped))
      offset = offset - rawTileLen * ceil(offset / rawTileLen);

    bool skipToOffset = true;

    for (size_t i = 0; i < pointsCount - 1; ++i)
    {
      m2::PointD dir = points[i + 1] - points[i];
      dir *= 1.0 / dir.Length(m2::PointD(0, 0));
      m2::PointD norm(-dir.y, dir.x);

      /// The length of the current segment.
      float segLen = points[i + 1].Length(points[i]);
      /// The remaining length of the segment
      float segLenRemain = segLen;

      if (skipToOffset)
      {
        offset -= segLen;
        if (offset >= 0)
          continue;
        else
        {
          skipToOffset = false;
          segLenRemain = -offset;
        }
      }

      /// Geometry width. It's 1px wider than the pattern width.
      int geomWidth = static_cast<int>(pen->m_info.m_w) + 4 - 2 * aaShift();
      float geomHalfWidth =  geomWidth / 2.0;

      /// Starting point of the tiles on this segment
      m2::PointF rawTileStartPt = points[i] + dir * (segLen - segLenRemain);

      /// Tiling procedes as following :
      /// The leftmost tile goes antialiased at left and non-antialiased at right.
      /// The inner tiles goes non-antialiased at both sides.
      /// The rightmost tile goes non-antialised at left and antialiased at right side.

      /// Length of the actual pattern data being tiling(without antialiasing zones).
      rawTileLen = 0;

      GeometryPipeline & p = pipeline(pen->m_pipelineID);

      shared_ptr<gl::BaseTexture> texture = p.texture();

      if (!texture)
      {
        LOG(LDEBUG, ("returning as no texture is reserved"));
        return;
      }

      float texMaxY = pen->m_texRect.maxY() - aaShift();
      float texMinY = pen->m_texRect.minY() + aaShift();

      m2::PointF const fNorm = norm * geomHalfWidth;  // enough to calc it once

      while (segLenRemain > 0)
      {
        rawTileLen = pen->m_isWrapped
            ? segLen
            : std::min(((float)pen->rawTileLen() - rawTileStartLen), segLenRemain);


        float texMinX = pen->m_isWrapped ? 0 : pen->m_texRect.minX() + 2 + rawTileStartLen;
        float texMaxX = texMinX + rawTileLen;

        rawTileStartLen += rawTileLen;
        if (rawTileStartLen >= pen->rawTileLen())
          rawTileStartLen -= pen->rawTileLen();
        ASSERT(rawTileStartLen < pen->rawTileLen(), ());

        m2::PointF rawTileEndPt(rawTileStartPt.x + dir.x * rawTileLen, rawTileStartPt.y + dir.y * rawTileLen);

        m2::PointF coords[4] =
        {
          // vng: i think this "rawTileStartPt + fNorm" reading better, isn't it?
          m2::PointF(rawTileStartPt.x + fNorm.x, rawTileStartPt.y + fNorm.y),
          m2::PointF(rawTileStartPt.x - fNorm.x, rawTileStartPt.y - fNorm.y),
          m2::PointF(rawTileEndPt.x - fNorm.x, rawTileEndPt.y - fNorm.y),
          m2::PointF(rawTileEndPt.x + fNorm.x, rawTileEndPt.y + fNorm.y)
        };

        m2::PointF texCoords[4] =
        {
          texture->mapPixel(m2::PointF(texMinX, texMinY)),
          texture->mapPixel(m2::PointF(texMinX, texMaxY)),
          texture->mapPixel(m2::PointF(texMaxX, texMaxY)),
          texture->mapPixel(m2::PointF(texMaxX, texMinY))
        };

        m2::PointF normals[4] =
        {
          m2::PointF(0, 0),
          m2::PointF(0, 0),
          m2::PointF(0, 0),
          m2::PointF(0, 0)
        };

        addTexturedFan(coords,
                       normals,
                       texCoords,
                       4,
                       depth,
                       pen->m_pipelineID);

        segLenRemain -= rawTileLen;

        rawTileStartPt = rawTileEndPt;
      }

      bool isColorJoin = pen->m_isSolid ? true : pen->m_info.atDashOffset(rawTileLen);

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

        GeometryPipeline & p = pipeline(pen->m_pipelineID);

        shared_ptr<gl::BaseTexture> texture = p.texture();

        if (!texture)
        {
          LOG(LDEBUG, ("returning as no texture is reserved"));
          return;
        }

        m2::PointF joinSegTex[3] =
        {
          texture->mapPixel(pen->m_centerColorPixel),
          texture->mapPixel(pen->m_borderColorPixel),
          texture->mapPixel(pen->m_borderColorPixel)
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

          m2::PointF joinSegNormals[3] =
          {
            m2::PointF(0, 0),
            m2::PointF(0, 0),
            m2::PointF(0, 0)
          };

          addTexturedFan(joinSeg,
                         joinSegNormals,
                         joinSegTex,
                         3,
                         depth,
                         pen->m_pipelineID);

          prevStartVec = startVec;
        }
      }
    }
  }

  void PathRenderer::drawSolidPath(m2::PointD const * points, size_t pointsCount, double offset, Pen const * pen, double depth)
  {
    ASSERT(pen->m_isSolid, ());

    for (size_t i = 0; i < pointsCount - 1; ++i)
    {
      m2::PointD dir = points[i + 1] - points[i];
      double len = dir.Length(m2::PointD(0, 0));
      dir *= 1.0 / len;
      m2::PointD norm(-dir.y, dir.x);
      m2::PointD const & nextPt = points[i + 1];

      float geomHalfWidth = (pen->m_info.m_w + 4 - aaShift() * 2) / 2.0;

      float texMinX = pen->m_texRect.minX() + 1;
      float texMaxX = pen->m_texRect.maxX() - 1;

      float texMinY = pen->m_texRect.maxY() - aaShift();
      float texMaxY = pen->m_texRect.minY() + aaShift();

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

      GeometryPipeline & p = pipeline(pen->m_pipelineID);

      shared_ptr<gl::BaseTexture> texture = p.texture();

      if (!texture)
      {
        LOG(LDEBUG, ("returning as no texture is reserved"));
        return;
      }

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

      m2::PointF normal(0, 0);

      addTexturedStripStrided(coords, sizeof(m2::PointF),
                              &normal, 0,
                              texCoords, sizeof(m2::PointF),
                              8,
                              depth,
                              pen->m_pipelineID);
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
    {
      LOG(LINFO, ("drawing ", m_pathCount, " pathes, ", m_pointsCount, " points total"));
    }
    base_t::endFrame();
  }
}


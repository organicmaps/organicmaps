#include "path_renderer.hpp"
#include "resource.hpp"
#include "pen.hpp"
#include "resource_cache.hpp"
#include "path_view.hpp"

#include "opengl/base_texture.hpp"

#include "../base/logging.hpp"

namespace graphics
{
  PathRenderer::Params::Params()
    : m_drawPathes(true),
      m_useNormals(true)
  {}

  PathRenderer::PathRenderer(Params const & p)
    : base_t(p),
      m_drawPathes(p.m_drawPathes),
      m_useNormals(p.m_useNormals)
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

    if (!pen->m_info.m_icon.m_name.empty())
      drawSymbolPath(pts, ptsCount, offset, pen, depth);
    else
      if (pen->m_isSolid)
        drawSolidPath(pts, ptsCount, offset, pen, depth);
      else
        drawStipplePath(pts, ptsCount, offset, pen, depth);
  }

  void PathRenderer::drawSymbolPath(m2::PointD const * pts, size_t ptsCount, double offset, Pen const * pen, double depth)
  {
    PathView pv(pts, ptsCount);

    PathPoint pt = pv.front();

    double step = pen->m_info.m_step;

    offset += pen->m_info.m_offset;

    if (offset < 0)
      offset = fmod(offset, step);

    pt = pv.offsetPoint(pt, offset);

    m2::RectU texRect = pen->m_texRect;
    texRect.Inflate(-1, -1);

    double const w = texRect.SizeX();
    double const h = texRect.SizeY();

    double const hw = w / 2.0;
    double const hh = h / 2.0;

    /// do not render start symbol if it's
    /// completely outside the first segment.
    if (offset + w < 0)
    {
      pv.offsetPoint(pt, step);
      offset += step;
    }

    shared_ptr<gl::BaseTexture> tex = pipeline(pen->m_pipelineID).texture();

    while (true)
    {
      PivotPoint pvPt = pv.findPivotPoint(pt, hw - 1, 0);

      if (pvPt.m_pp.m_i == -1)
        break;

      ang::AngleD ang = pvPt.m_angle;

      m2::PointD pts[4];

      pts[0] = pvPt.m_pp.m_pt.Move(-hw, ang.sin(), ang.cos());
      pts[0] = pts[0].Move(hh + 1, -ang.cos(), ang.sin());

      pts[1] = pts[0].Move(w, ang.sin(), ang.cos());
      pts[2] = pts[0].Move(-h, -ang.cos(), ang.sin());
      pts[3] = pts[2].Move(w, ang.sin(), ang.cos());

      m2::PointF texPts[4] =
      {
        tex->mapPixel(m2::PointF(texRect.minX(), texRect.minY())),
        tex->mapPixel(m2::PointF(texRect.maxX(), texRect.minY())),
        tex->mapPixel(m2::PointF(texRect.minX(), texRect.maxY())),
        tex->mapPixel(m2::PointF(texRect.maxX(), texRect.maxY()))
      };

      m2::PointF normal(0, 0);

      addTexturedStripStrided(pts, sizeof(m2::PointD),
                              &normal, 0,
                              texPts, sizeof(m2::PointF),
                              4,
                              depth,
                              pen->m_pipelineID);

      pt = pv.offsetPoint(pvPt.m_pp, pen->m_info.m_step);

      if (pt.m_i == -1)
        break;
    }
  }

  void PathRenderer::drawStipplePath(m2::PointD const * points, size_t pointsCount, double offset, Pen const * pen, double depth)
  {
    bool const hasRoundJoin = (pen->m_info.m_join == pen->m_info.ERoundJoin);
//    bool const hasBevelJoin = (pen->m_info.m_join == pen->m_info.EBevelJoin);
    bool const hasJoin = (pen->m_info.m_join != pen->m_info.ENoJoin);

    float rawTileStartLen = 0;

    float rawTileLen = (float)pen->rawTileLen();

    if (offset < 0)
      offset = offset - rawTileLen * ceil(offset / rawTileLen);

    bool skipToOffset = true;

    /// Geometry width. It's 1px wider than the pattern width.
    float const geomWidth = pen->m_info.m_w + 4 - 2 * aaShift();
    float const geomHalfWidth = geomWidth / 2.0;

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
        rawTileLen = std::min(((float)pen->rawTileLen() - rawTileStartLen), segLenRemain);

        float texMinX = pen->m_texRect.minX() + 2 + rawTileStartLen;
        float texMaxX = texMinX + rawTileLen;

        rawTileStartLen += rawTileLen;
        if (rawTileStartLen >= pen->rawTileLen())
          rawTileStartLen -= pen->rawTileLen();
        ASSERT(rawTileStartLen < pen->rawTileLen(), ());

        m2::PointF rawTileEndPt(rawTileStartPt.x + dir.x * rawTileLen, rawTileStartPt.y + dir.y * rawTileLen);

        m2::PointF coords[4];

        if (m_useNormals)
        {
          coords[0] = rawTileStartPt;
          coords[1] = rawTileStartPt;
          coords[2] = rawTileEndPt;
          coords[3] = rawTileEndPt;
        }
        else
        {
          coords[0] = rawTileStartPt + fNorm;
          coords[1] = rawTileStartPt - fNorm;
          coords[2] = rawTileEndPt - fNorm;
          coords[3] = rawTileEndPt + fNorm;
        };

        m2::PointF texCoords[4] =
        {
          texture->mapPixel(m2::PointF(texMinX, texMinY)),
          texture->mapPixel(m2::PointF(texMinX, texMaxY)),
          texture->mapPixel(m2::PointF(texMaxX, texMaxY)),
          texture->mapPixel(m2::PointF(texMaxX, texMinY))
        };

        m2::PointF normals[4];

        if (m_useNormals)
        {
          normals[0] = fNorm;
          normals[1] = -fNorm;
          normals[2] = -fNorm;
          normals[3] = fNorm;
        }
        else
          memset(normals, 0, sizeof(normals));

        addTexturedFan(coords,
                       normals,
                       texCoords,
                       4,
                       depth,
                       pen->m_pipelineID);

        segLenRemain -= rawTileLen;

        rawTileStartPt = rawTileEndPt;
      }

      bool isColorJoin = hasJoin && pen->m_info.atDashOffset(rawTileLen);

      /// Adding geometry for a line join between previous and current segment.
      if ((i != pointsCount - 2) && isColorJoin)
      {
        m2::PointD nextDir = points[i + 2] - points[i + 1];
        nextDir *= 1.0 / nextDir.Length(m2::PointD(0, 0));
        m2::PointD nextNorm(-nextDir.y, nextDir.x);

        /// Computing the sin of angle between directions.
        double alphaSin = dir.x * nextDir.y - dir.y * nextDir.x;
        double alphaCos = dir.x * nextDir.x + dir.y * nextDir.y;
        double alpha = atan2(alphaSin, alphaCos);

        int angleSegCount = 1; // bevel join - 1 segment
        if (hasRoundJoin)
          angleSegCount= int(ceil(fabs(alpha) / (math::pi / 6)));

        double angleStep = alpha / angleSegCount;

        m2::PointD startVec;

        if (alpha > 0)
        {
          /// The outer side is on the prevNorm direction.
          startVec = -norm;
        }
        else
        {
          /// The outer side is on the -prevNorm direction
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

          m2::PointF joinSeg[3];
          m2::PointF joinSegNormals[3];

          if (m_useNormals)
          {
            joinSeg[0] = points[i + 1];
            joinSeg[1] = points[i + 1];
            joinSeg[2] = points[i + 1];

            joinSegNormals[0] = m2::PointF(0, 0);
            joinSegNormals[1] = startVec * geomHalfWidth;
            joinSegNormals[2] = prevStartVec * geomHalfWidth;
          }
          else
          {
            joinSeg[0] = m2::PointF(points[i + 1]);
            joinSeg[1] = m2::PointF(points[i + 1] + startVec * geomHalfWidth);
            joinSeg[2] = m2::PointF(points[i + 1] + prevStartVec * geomHalfWidth);

            memset(joinSegNormals, 0, sizeof(joinSegNormals));
          }

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

    bool const hasRoundCap = (pen->m_info.m_cap == pen->m_info.ERoundCap);
    bool const hasSquareCap = (pen->m_info.m_cap == pen->m_info.ESquareCap);
    bool const hasRoundJoin = (pen->m_info.m_join == pen->m_info.ERoundJoin);
    bool const hasBevelJoin = (pen->m_info.m_join == pen->m_info.EBevelJoin);

    float geomHalfWidth = (pen->m_info.m_w + 4 - aaShift() * 2) / 2.0;

    m2::PointD dir = points[1] - points[0];
    double len = dir.Length(m2::PointD(0, 0));
    dir *= 1.0 / len;
    m2::PointD const norm(-dir.y, dir.x);
    m2::PointD fNorm = norm * geomHalfWidth;
    m2::PointD fDir(fNorm.y, -fNorm.x);

    m2::PointD fNormNextSeg;
    m2::PointD fDirNextSeg;

    for (size_t i = 0; i < pointsCount - 1; ++i)
    {      
      bool const leftIsCap  = i == 0;
      bool const rightIsCap = i == (pointsCount - 2);

      if (!leftIsCap)
      {
        fNorm = fNormNextSeg;
        fDir = fDirNextSeg;
      }

      m2::PointD const & nextPt = points[i + 1];

      if (!rightIsCap)
      {
        m2::PointD dirNextSeg = points[i + 2] - points[i + 1];
        double lenNextSeg = dirNextSeg.Length(m2::PointD(0, 0));
        dirNextSeg *= 1.0 / lenNextSeg;
        m2::PointD normNextSeg(-dirNextSeg.y, dirNextSeg.x);
        fNormNextSeg = normNextSeg * geomHalfWidth;
        fDirNextSeg = m2::PointD(fNormNextSeg.y, -fNormNextSeg.x);
      }

      float texMinX = pen->m_texRect.minX() + aaShift();
      float texMaxX = pen->m_texRect.maxX() - aaShift();

      float texMinY = pen->m_texRect.minY() + aaShift();
      float texMaxY = pen->m_texRect.maxY() - aaShift();

      float texCenterX = (texMinX + texMaxX) / 2;

      int numPoints = 4;

      if (leftIsCap && (hasRoundCap || hasSquareCap))
        numPoints += 2;
      if ((rightIsCap && (hasRoundCap || hasSquareCap))
       || (!rightIsCap && (hasRoundJoin || hasBevelJoin)))
        numPoints += 2;

      int cur = 0;

      ASSERT(numPoints <= 8, ("numPoints is more than 8, won't fit in array"));
      m2::PointF coords[8];

      if (leftIsCap && (hasRoundCap || hasSquareCap))
      {
        coords[cur++] = points[i] - fDir + fNorm;
        coords[cur++] = points[i] - fDir - fNorm;
      }

      coords[cur++] = points[i] + fNorm;
      coords[cur++] = points[i] - fNorm;
      coords[cur++] = nextPt + fNorm;
      coords[cur++] = nextPt - fNorm;

      if ((rightIsCap && (hasRoundCap || hasSquareCap))
       || (!rightIsCap && hasRoundJoin))
      {
        coords[cur++] = nextPt + fDir + fNorm;
        coords[cur++] = nextPt + fDir - fNorm;
      }
      else if (!rightIsCap && hasBevelJoin)
      {
        coords[cur++] = points[i + 1] + fNormNextSeg;
        coords[cur++] = points[i + 1] - fNormNextSeg;
      }

      GeometryPipeline & p = pipeline(pen->m_pipelineID);

      shared_ptr<gl::BaseTexture> texture = p.texture();

      if (!texture)
      {
        LOG(LDEBUG, ("returning as no texture is reserved"));
        return;
      }

      m2::PointF texCoords[8];
      cur = 0;

      if (leftIsCap && hasRoundCap)
      {
        texCoords[cur++] = texture->mapPixel(m2::PointF(texMinX, texMinY));
        texCoords[cur++] = texture->mapPixel(m2::PointF(texMinX, texMaxY));
      }
      else if (leftIsCap && hasSquareCap)
      {
        texCoords[cur++] = texture->mapPixel(m2::PointF(texCenterX, texMinY));
        texCoords[cur++] = texture->mapPixel(m2::PointF(texCenterX, texMaxY));
      }

      texCoords[cur++] = texture->mapPixel(m2::PointF(texCenterX, texMinY));
      texCoords[cur++] = texture->mapPixel(m2::PointF(texCenterX, texMaxY));
      texCoords[cur++] = texture->mapPixel(m2::PointF(texCenterX, texMinY));
      texCoords[cur++] = texture->mapPixel(m2::PointF(texCenterX, texMaxY));

      if ((rightIsCap && hasRoundCap)
       || (!rightIsCap && hasRoundJoin))
      {
        texCoords[cur++] = texture->mapPixel(m2::PointF(texMinX, texMinY));
        texCoords[cur++] = texture->mapPixel(m2::PointF(texMinX, texMaxY));
      }
      else if ((rightIsCap && hasSquareCap)
            || (!rightIsCap && hasBevelJoin))
      {
        texCoords[cur++] = texture->mapPixel(m2::PointF(texCenterX, texMinY));
        texCoords[cur++] = texture->mapPixel(m2::PointF(texCenterX, texMaxY));
      }

      m2::PointF normal(0, 0);

      addTexturedStripStrided(coords, sizeof(m2::PointF),
                              &normal, 0,
                              texCoords, sizeof(m2::PointF),
                              numPoints,
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

  void PathRenderer::setUseNormals(bool flag)
  {
    m_useNormals = flag;
  }
}


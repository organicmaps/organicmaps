#include "../base/SRC_FIRST.hpp"

#include "path_renderer.hpp"
#include "resource_style.hpp"
#include "skin.hpp"

namespace yg
{
  namespace gl
  {

    PathRenderer::PathRenderer(base_t::Params const & params) : base_t(params)
    {}

    void PathRenderer::drawPath(m2::PointD const * points, size_t pointsCount, uint32_t styleID, double depth)
    {
      ASSERT_GREATER_OR_EQUAL(pointsCount, 2, ());
      ResourceStyle const * style(skin()->fromID(styleID));

      ASSERT(style->m_cat == ResourceStyle::ELineStyle, ());
      LineStyle const * lineStyle = static_cast<LineStyle const *>(style);

      if (lineStyle->m_isSolid)
      {
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
      else
        base_t::drawPath(points, pointsCount, styleID, depth);
    }
  }
}


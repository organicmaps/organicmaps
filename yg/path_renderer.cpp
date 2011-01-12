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
          dir *= 1.0 / dir.Length(m2::PointD(0, 0));
          m2::PointD norm(-dir.y, dir.x);

          float segLen = points[i + 1].Length(points[i]);
          float geomHalfWidth = (lineStyle->m_penInfo.m_w + 4 - aaShift() * 2) / 2.0;

          float texMinX = lineStyle->m_texRect.minX() + 1;
          float texMaxX = lineStyle->m_texRect.maxX() - 1;

          float texMinY = lineStyle->m_texRect.maxY() - aaShift();
          float texMaxY = lineStyle->m_texRect.minY() + aaShift();

          float texCenterX = (texMinX + texMaxX) / 2;

          m2::PointF const fNorm = norm * geomHalfWidth;  // enough to calc it once
          m2::PointF const fDir(fNorm.y, -fNorm.x);

          m2::PointF coords[12] =
          {
            /// left round cap
            m2::PointF(points[i].x - fDir.x + fNorm.x, points[i].y - fDir.y + fNorm.y),
            m2::PointF(points[i].x - fDir.x - fNorm.x, points[i].y - fDir.y - fNorm.y),
            m2::PointF(points[i].x - fNorm.x, points[i].y - fNorm.y),
            m2::PointF(points[i].x + fNorm.x, points[i].y + fNorm.y),
            /// inner part
            m2::PointF(points[i].x + fNorm.x, points[i].y + fNorm.y),
            m2::PointF(points[i].x - fNorm.x, points[i].y - fNorm.y),
            m2::PointF(points[i + 1].x - fNorm.x, points[i + 1].y - fNorm.y),
            m2::PointF(points[i + 1].x + fNorm.x, points[i + 1].y + fNorm.y),
            /// right round cap
            m2::PointF(points[i + 1].x + fNorm.x, points[i + 1].y + fNorm.y),
            m2::PointF(points[i + 1].x - fNorm.x, points[i + 1].y - fNorm.y),
            m2::PointF(points[i + 1].x + fDir.x - fNorm.x, points[i + 1].y + fDir.y - fNorm.y),
            m2::PointF(points[i + 1].x + fDir.x + fNorm.x, points[i + 1].y + fDir.y + fNorm.y)
          };

          shared_ptr<BaseTexture> texture = skin()->pages()[lineStyle->m_pageID]->texture();

          m2::PointF texCoords[12] =
          {
            /// left round cap
            texture->mapPixel(m2::PointF(texMinX, texMinY)),
            texture->mapPixel(m2::PointF(texMinX, texMaxY)),
            texture->mapPixel(m2::PointF(texCenterX, texMaxY)),
            texture->mapPixel(m2::PointF(texCenterX, texMinY)),
            /// inner part
            texture->mapPixel(m2::PointF(texCenterX, texMinY)),
            texture->mapPixel(m2::PointF(texCenterX, texMaxY)),
            texture->mapPixel(m2::PointF(texCenterX, texMaxY)),
            texture->mapPixel(m2::PointF(texCenterX, texMinY)),
            /// right round cap
            texture->mapPixel(m2::PointF(texCenterX, texMinY)),
            texture->mapPixel(m2::PointF(texCenterX, texMaxY)),
            texture->mapPixel(m2::PointF(texMaxX, texMaxY)),
            texture->mapPixel(m2::PointF(texMaxX, texMinY))
          };

          addTexturedVertices(coords, texCoords, 4, depth, lineStyle->m_pageID);
          addTexturedVertices(coords + 4, texCoords + 4, 4, depth, lineStyle->m_pageID);
          addTexturedVertices(coords + 8, texCoords + 8, 4, depth, lineStyle->m_pageID);
        }
      }
      else
        base_t::drawPath(points, pointsCount, styleID, depth);
    }
  }
}


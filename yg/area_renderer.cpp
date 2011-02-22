#include "area_renderer.hpp"
#include "resource_style.hpp"
#include "skin.hpp"
#include "../base/logging.hpp"

namespace yg
{
  namespace gl
  {
    AreaRenderer::Params::Params()
      : m_drawAreas(true)
    {
    }

    AreaRenderer::AreaRenderer(Params const & params)
      : base_t(params),
      m_drawAreas(params.m_drawAreas)
    {}

    void AreaRenderer::beginFrame()
    {
      base_t::beginFrame();
      m_areasCount = 0;
      m_trianglesCount = 0;
    };

    void AreaRenderer::endFrame()
    {
      if (isDebugging())
        LOG(LINFO, ("Drawing ", m_areasCount, " areas, ", m_trianglesCount, " triangles total"));
      base_t::endFrame();
    }

    void AreaRenderer::drawTrianglesList(m2::PointD const * points, size_t pointsCount, uint32_t styleID, double depth)
    {
      ++m_areasCount;
      m_trianglesCount += pointsCount;

      if (!m_drawAreas)
        return;

      ResourceStyle const * style = skin()->fromID(styleID);

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

      skin()->pages()[style->m_pageID]->texture()->mapPixel(texX, texY);

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

        m2::PointF texCoord(texX, texY);
        addTexturedListStrided(&points[batchOffset], sizeof(m2::PointD), &texCoord, 0, batchSize, depth, style->m_pageID);

        batchOffset += batchSize;
        pointsLeft -= batchSize;

        if (needToFlush)
          flush(style->m_pageID);

        if (pointsLeft == 0)
          break;
      }
    }

  }
}

#include "area_renderer.hpp"
#include "resource_style.hpp"
#include "skin.hpp"
#include "skin_page.hpp"
#include "base_texture.hpp"

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
    }

    void AreaRenderer::endFrame()
    {
      if (isDebugging())
      {
        LOG(LINFO, ("drawing ", m_areasCount, " areas, ", m_trianglesCount, " triangles total"));
      }
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
        LOG(LINFO, ("drawArea: styleID=", styleID, " wasn't found on current skin."));
        return;
      }

      if (!hasRoom(pointsCount, pointsCount, style->m_pipelineID))
        flush(style->m_pipelineID);

      ASSERT_GREATER_OR_EQUAL(pointsCount, 2, ());

      float texX = style->m_texRect.minX() + 1.0f;
      float texY = style->m_texRect.minY() + 1.0f;

      shared_ptr<BaseTexture> texture = skin()->getPage(style->m_pipelineID)->texture();

      if (!texture)
      {
        LOG(LDEBUG, ("returning as no texture is reserved"));
        return;
      }

      texture->mapPixel(texX, texY);

      size_t pointsLeft = pointsCount;
      size_t batchOffset = 0;

      while (true)
      {
        size_t batchSize = pointsLeft;

        int vLeft = verticesLeft(style->m_pipelineID);
        int iLeft = indicesLeft(style->m_pipelineID);

        if ((vLeft == -1) || (iLeft == -1))
          return;

        if (batchSize > vLeft)
          /// Rounding to the boundary of 3 vertices
          batchSize = vLeft / 3 * 3;

        if (batchSize > iLeft)
          batchSize = iLeft / 3 * 3;

        bool needToFlush = (batchSize < pointsLeft);

        m2::PointF texCoord(texX, texY);
        m2::PointF normal(0, 0);

        addTexturedListStrided(&points[batchOffset], sizeof(m2::PointD),
                               &normal, 0,
                               &texCoord, 0,
                               batchSize,
                               depth,
                               style->m_pipelineID);

        batchOffset += batchSize;
        pointsLeft -= batchSize;

        if (needToFlush)
          flush(style->m_pipelineID);

        if (pointsLeft == 0)
          break;
      }
    }

  }
}

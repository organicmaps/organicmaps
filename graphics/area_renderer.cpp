#include "area_renderer.hpp"
#include "brush.hpp"
#include "resource_cache.hpp"

#include "opengl/base_texture.hpp"

#include "../base/logging.hpp"

namespace graphics
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
    base_t::endFrame();
  }

  void AreaRenderer::drawTrianglesFan(m2::PointF const * points,
                                      size_t pointsCount,
                                      uint32_t resID,
                                      double depth)
  {
    ++m_areasCount;
    m_trianglesCount += (pointsCount - 2);

    if (!m_drawAreas)
      return;

    Resource const * res = base_t::fromID(resID);

    ASSERT(res->m_cat == Resource::EBrush, ("triangleFan should be rendered with Brush resource"));

    if (res == 0)
    {
      LOG(LDEBUG, ("drawTrianglesFan: resID=", resID, " wasn't found on current skin."));
      return;
    }

    ASSERT_GREATER_OR_EQUAL(pointsCount, 2, ());

    float texX = res->m_texRect.minX() + 1.0f;
    float texY = res->m_texRect.minY() + 1.0f;

    GeometryPipeline & p = pipeline(res->m_pipelineID);
    shared_ptr<gl::BaseTexture> texture = p.texture();

    if (!texture)
    {
      LOG(LDEBUG, ("returning as no texture is reserved"));
      return;
    }

    texture->mapPixel(texX, texY);

    m2::PointF texCoord(texX, texY);
    m2::PointF normal(0, 0);

    addTexturedFanStrided(points, sizeof(m2::PointF),
                          &normal, 0,
                          &texCoord, 0,
                          pointsCount,
                          depth,
                          res->m_pipelineID);
  }

  void AreaRenderer::drawTrianglesList(m2::PointD const * points, size_t pointsCount, uint32_t resID, double depth)
  {
    ++m_areasCount;
    m_trianglesCount += pointsCount / 3;

    if (!m_drawAreas)
      return;

    Resource const * res = base_t::fromID(resID);

    ASSERT(res->m_cat == Resource::EBrush, ("area should be rendered with Brush resource"));

    if (res == 0)
    {
      LOG(LDEBUG, ("drawArea: resID=", resID, " wasn't found on current skin."));
      return;
    }

    if (!hasRoom(pointsCount, pointsCount, res->m_pipelineID))
      flush(res->m_pipelineID);

    ASSERT_GREATER_OR_EQUAL(pointsCount, 2, ());

    float texX = res->m_texRect.minX() + 1.0f;
    float texY = res->m_texRect.minY() + 1.0f;

    GeometryPipeline & p = pipeline(res->m_pipelineID);
    shared_ptr<gl::BaseTexture> texture = p.texture();

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

      int vLeft = verticesLeft(res->m_pipelineID);
      int iLeft = indicesLeft(res->m_pipelineID);

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
                             res->m_pipelineID);

      batchOffset += batchSize;
      pointsLeft -= batchSize;

      if (needToFlush)
        flush(res->m_pipelineID);

      if (pointsLeft == 0)
        break;
    }
  }
}

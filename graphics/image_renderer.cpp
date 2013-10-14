#include "image_renderer.hpp"
#include "resource.hpp"

#include "opengl/base_texture.hpp"

#include "../base/assert.hpp"
#include "../base/macros.hpp"

namespace graphics
{
  ImageRenderer::ImageRenderer(base_t::Params const & p)
    : base_t(p)
  {}

  void ImageRenderer::drawImage(math::Matrix<double, 3, 3> const & m,
                                uint32_t resID,
                                double depth)
  {
    Resource const * res(base_t::fromID(resID));

    if (res == 0)
    {
      LOG(LINFO, ("drawImage: resID=", resID, "wasn't found on current skin"));
      return;
    }

    ASSERT(res->m_cat == Resource::EImage, ());

    m2::RectI texRect(res->m_texRect);

    m2::PointF pts[6] =
    {
      m2::PointF(m2::PointD(0, 0) * m),
      m2::PointF(m2::PointD(texRect.SizeX(), 0) * m),
      m2::PointF(m2::PointD(texRect.SizeX(), texRect.SizeY()) * m),
      m2::PointF(m2::PointD(texRect.SizeX(), texRect.SizeY()) * m),
      m2::PointF(m2::PointD(0, texRect.SizeY()) * m),
      m2::PointF(m2::PointD(0, 0) * m)
    };

    GeometryPipeline & p = pipeline(res->m_pipelineID);

    shared_ptr<gl::BaseTexture> const & texture = p.texture();

    m2::PointF texPts[6] =
    {
      m2::PointF(texRect.minX(), texRect.minY()),
      m2::PointF(texRect.maxX(), texRect.minY()),
      m2::PointF(texRect.maxX(), texRect.maxY()),
      m2::PointF(texRect.maxX(), texRect.maxY()),
      m2::PointF(texRect.minX(), texRect.maxY()),
      m2::PointF(texRect.minX(), texRect.minY())
    };

    for (unsigned i = 0; i < ARRAY_SIZE(texPts); ++i)
      texture->mapPixel(texPts[i].x, texPts[i].y);

    m2::PointF normal(0, 0);

    addTexturedListStrided(pts, sizeof(m2::PointF),
                           &normal, 0,
                           texPts, sizeof(m2::PointF),
                           6,
                           depth,
                           res->m_pipelineID);
  }
}

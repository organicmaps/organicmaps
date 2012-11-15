#include "image_renderer.hpp"
#include "resource_style.hpp"
#include "skin.hpp"

#include "opengl/base_texture.hpp"

#include "../base/assert.hpp"
#include "../base/macros.hpp"

namespace graphics
{
  ImageRenderer::ImageRenderer(base_t::Params const & p)
    : base_t(p)
  {}

  void ImageRenderer::drawImage(math::Matrix<double, 3, 3> const & m,
                                uint32_t styleID,
                                double depth)
  {
    ResourceStyle const * style(skin()->fromID(styleID));

    if (style == 0)
    {
      LOG(LINFO, ("drawImage: styleID=", styleID, "wasn't found on current skin"));
      return;
    }

    ASSERT(style->m_cat == ResourceStyle::EImageStyle, ());

    m2::RectI texRect(style->m_texRect);
    texRect.Inflate(-1, -1);

    m2::PointF pts[6] =
    {
      m2::PointF(m2::PointD(-1, -1) * m),
      m2::PointF(m2::PointD(texRect.SizeX() - 1, -1) * m),
      m2::PointF(m2::PointD(texRect.SizeX() - 1, texRect.SizeY() - 1) * m),
      m2::PointF(m2::PointD(texRect.SizeX() - 1, texRect.SizeY() - 1) * m),
      m2::PointF(m2::PointD(-1, texRect.SizeY() - 1) * m),
      m2::PointF(m2::PointD(-1, -1) * m)
    };

    shared_ptr<gl::BaseTexture> const & texture = skin()->page(style->m_pipelineID)->texture();

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
                           style->m_pipelineID);
  }
}

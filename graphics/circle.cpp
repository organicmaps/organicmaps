#include "base/SRC_FIRST.hpp"

#include "graphics/circle.hpp"
#include "graphics/opengl/data_traits.hpp"
#include "graphics/agg_traits.hpp"

#include "base/math.hpp"

namespace graphics
{
  Circle::Info::Info(double radius,
                     Color const & color,
                     bool isOutlined,
                     double outlineWidth,
                     Color const & outlineColor)
   : Resource::Info(Resource::ECircle),
     m_radius(my::rounds(radius)),
     m_color(color),
     m_isOutlined(isOutlined),
     m_outlineWidth(my::rounds(outlineWidth)),
     m_outlineColor(outlineColor)
  {
    if (!m_isOutlined)
    {
      m_outlineWidth = 0;
      m_outlineColor = graphics::Color(0, 0, 0, 0);
    }
  }

  Circle::Info::Info()
    : Resource::Info(Resource::ECircle),
      m_radius(0),
      m_color(0, 0, 0, 0),
      m_isOutlined(false),
      m_outlineWidth(0),
      m_outlineColor()
  {}

  bool Circle::Info::lessThan(Resource::Info const * r) const
  {
    if (m_category != r->m_category)
      return m_category < r->m_category;

    Circle::Info const * ci = static_cast<Circle::Info const *>(r);

    if (m_radius != ci->m_radius)
      return m_radius < ci->m_radius;
    if (m_color != ci->m_color)
      return m_color < ci->m_color;
    if (m_isOutlined != ci->m_isOutlined)
      return m_isOutlined < ci->m_isOutlined;
    if (m_outlineWidth != ci->m_outlineWidth)
      return m_outlineWidth < ci->m_outlineWidth;
    if (m_outlineColor != ci->m_outlineColor)
      return m_outlineColor < ci->m_outlineColor;

    return false;
  }

  Resource::Info const & Circle::Info::cacheKey() const
  {
    return *this;
  }

  m2::PointU const Circle::Info::resourceSize() const
  {
    unsigned r = m_isOutlined ? m_radius + m_outlineWidth : m_radius;
    return m2::PointU(r * 2 + 4, r * 2 + 4);
  }

  Resource * Circle::Info::createResource(m2::RectU const & texRect,
                                          uint8_t pipelineID) const
  {
    return new Circle(texRect,
                      pipelineID,
                      *this);
  }

  Circle::Circle(m2::RectU const & texRect,
                 int pipelineID,
                 Info const & info)
    : Resource(ECircle, texRect, pipelineID),
      m_info(info)
  {}

  void Circle::render(void * dst)
  {
    m2::RectU const & rect = m_texRect;

    DATA_TRAITS::view_t v = gil::interleaved_view(
          rect.SizeX(), rect.SizeY(),
          (DATA_TRAITS::pixel_t*)dst,
          sizeof(DATA_TRAITS::pixel_t) * rect.SizeX()
      );

    Circle::Info info = m_info;

    agg::rgba8 aggColor(info.m_color.r,
                        info.m_color.g,
                        info.m_color.b,
                        info.m_color.a);

    agg::rgba8 aggOutlineColor(info.m_outlineColor.r,
                               info.m_outlineColor.g,
                               info.m_outlineColor.b,
                               info.m_outlineColor.a);

    info.m_color /= DATA_TRAITS::channelScaleFactor;

    DATA_TRAITS::pixel_t gilColorTranslucent;

    gil::get_color(gilColorTranslucent, gil::red_t()) = info.m_color.r;
    gil::get_color(gilColorTranslucent, gil::green_t()) = info.m_color.g;
    gil::get_color(gilColorTranslucent, gil::blue_t()) = info.m_color.b;
    gil::get_color(gilColorTranslucent, gil::alpha_t()) = 0;

    info.m_outlineColor /= DATA_TRAITS::channelScaleFactor;

    DATA_TRAITS::pixel_t gilOutlineColorTranslucent;

    gil::get_color(gilOutlineColorTranslucent, gil::red_t()) = info.m_outlineColor.r;
    gil::get_color(gilOutlineColorTranslucent, gil::green_t()) = info.m_outlineColor.g;
    gil::get_color(gilOutlineColorTranslucent, gil::blue_t()) = info.m_outlineColor.b;
    gil::get_color(gilOutlineColorTranslucent, gil::alpha_t()) = 0;

    DATA_TRAITS::pixel_t gilColor = gilColorTranslucent;
    gil::get_color(gilColor, gil::alpha_t()) = info.m_color.a;

    DATA_TRAITS::pixel_t gilOutlineColor = gilOutlineColorTranslucent;
    gil::get_color(gilOutlineColor, gil::alpha_t()) = info.m_outlineColor.a;

    /// draw circle
    agg::rendering_buffer buf(
        (unsigned char *)&v(0, 0),
        rect.SizeX(),
        rect.SizeY(),
        rect.SizeX() * sizeof(DATA_TRAITS::pixel_t)
        );

    typedef AggTraits<DATA_TRAITS>::pixfmt_t agg_pixfmt_t;

    agg_pixfmt_t pixfmt(buf);
    agg::renderer_base<agg_pixfmt_t> rbase(pixfmt);

    if (info.m_isOutlined)
      gil::fill_pixels(v, gilOutlineColorTranslucent);
    else
      gil::fill_pixels(v, gilColorTranslucent);

    m2::PointD center(info.resourceSize());
    center *= 0.5;

    agg::scanline_u8 s;
    agg::rasterizer_scanline_aa<> rasterizer;

    agg::ellipse ell;

    unsigned radius = info.m_radius;
    if (info.m_isOutlined)
      radius += info.m_outlineWidth;

    ell.init(center.x,
             center.y,
             radius,
             radius,
             100);

    rasterizer.add_path(ell);

    agg::render_scanlines_aa_solid(rasterizer,
                                   s,
                                   rbase,
                                   info.m_isOutlined ? aggOutlineColor : aggColor);

    if (info.m_isOutlined)
    {
      /// drawing inner circle
      ell.init(center.x,
               center.y,
               info.m_radius,
               info.m_radius,
               100);

      rasterizer.reset();
      rasterizer.add_path(ell);

      agg::render_scanlines_aa_solid(rasterizer,
                                     s,
                                     rbase,
                                     aggColor);

    }
  }

  Resource::Info const * Circle::info() const
  {
    return &m_info;
  }
}


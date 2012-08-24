#include "resource_style.hpp"

#include "agg_traits.hpp"
#include "data_traits.hpp"

namespace yg
{
  CircleStyle::CircleStyle(m2::RectU const & texRect, int pipelineID, yg::CircleInfo const & ci)
    : ResourceStyle(ECircleStyle, texRect, pipelineID),
      m_ci(ci)
  {}

  void CircleStyle::render(void * dst)
  {
    yg::CircleInfo & circleInfo = m_ci;
    m2::RectU const & rect = m_texRect;

    DATA_TRAITS::view_t v = gil::interleaved_view(
          rect.SizeX(), rect.SizeY(),
          (DATA_TRAITS::pixel_t*)dst,
          sizeof(DATA_TRAITS::pixel_t) * rect.SizeX()
      );

    agg::rgba8 aggColor(circleInfo.m_color.r,
                        circleInfo.m_color.g,
                        circleInfo.m_color.b,
                        circleInfo.m_color.a);

    agg::rgba8 aggOutlineColor(circleInfo.m_outlineColor.r,
                               circleInfo.m_outlineColor.g,
                               circleInfo.m_outlineColor.b,
                               circleInfo.m_outlineColor.a);

    circleInfo.m_color /= DATA_TRAITS::channelScaleFactor;

    DATA_TRAITS::pixel_t gilColorTranslucent;

    gil::get_color(gilColorTranslucent, gil::red_t()) = circleInfo.m_color.r;
    gil::get_color(gilColorTranslucent, gil::green_t()) = circleInfo.m_color.g;
    gil::get_color(gilColorTranslucent, gil::blue_t()) = circleInfo.m_color.b;
    gil::get_color(gilColorTranslucent, gil::alpha_t()) = 0;

    circleInfo.m_outlineColor /= DATA_TRAITS::channelScaleFactor;

    DATA_TRAITS::pixel_t gilOutlineColorTranslucent;

    gil::get_color(gilOutlineColorTranslucent, gil::red_t()) = circleInfo.m_outlineColor.r;
    gil::get_color(gilOutlineColorTranslucent, gil::green_t()) = circleInfo.m_outlineColor.g;
    gil::get_color(gilOutlineColorTranslucent, gil::blue_t()) = circleInfo.m_outlineColor.b;
    gil::get_color(gilOutlineColorTranslucent, gil::alpha_t()) = 0;

    DATA_TRAITS::pixel_t gilColor = gilColorTranslucent;
    gil::get_color(gilColor, gil::alpha_t()) = circleInfo.m_color.a;

    DATA_TRAITS::pixel_t gilOutlineColor = gilOutlineColorTranslucent;
    gil::get_color(gilOutlineColor, gil::alpha_t()) = circleInfo.m_outlineColor.a;

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

    if (circleInfo.m_isOutlined)
      gil::fill_pixels(v, gilOutlineColorTranslucent);
    else
      gil::fill_pixels(v, gilColorTranslucent);

    m2::PointD center(circleInfo.patternSize());
    center *= 0.5;

    agg::scanline_u8 s;
    agg::rasterizer_scanline_aa<> rasterizer;

    agg::ellipse ell;

    ell.init(center.x,
             center.y,
             circleInfo.m_isOutlined ? circleInfo.m_radius + circleInfo.m_outlineWidth : circleInfo.m_radius,
             circleInfo.m_isOutlined ? circleInfo.m_radius + circleInfo.m_outlineWidth : circleInfo.m_radius,
             100);

    rasterizer.add_path(ell);

    agg::render_scanlines_aa_solid(rasterizer,
                                   s,
                                   rbase,
                                   circleInfo.m_isOutlined ? aggOutlineColor : aggColor);

    //DATA_TRAITS::pixel_t px = circleInfo.m_isOutlined ? gilOutlineColor : gilColor;

    /*
    // making alpha channel opaque
    for (size_t x = 2; x < v.width() - 2; ++x)
      for (size_t y = 2; y < v.height() - 2; ++y)
      {
        unsigned char alpha = gil::get_color(v(x, y), gil::alpha_t());

        float fAlpha = alpha / (float)DATA_TRAITS::maxChannelVal;

        if (alpha != 0)
        {
          gil::get_color(v(x, y), gil::red_t()) *= fAlpha;
          gil::get_color(v(x, y), gil::green_t()) *= fAlpha;
          gil::get_color(v(x, y), gil::blue_t()) *= fAlpha;

          gil::get_color(v(x, y), gil::alpha_t()) = DATA_TRAITS::maxChannelVal;
        }
      }
    */

    if (circleInfo.m_isOutlined)
    {
      /// drawing inner circle
      ell.init(center.x,
               center.y,
               circleInfo.m_radius,
               circleInfo.m_radius,
               100);

      rasterizer.reset();
      rasterizer.add_path(ell);

      agg::render_scanlines_aa_solid(rasterizer,
                                     s,
                                     rbase,
                                     aggColor);

      /*
      for (size_t x = 2; x < v.width() - 2; ++x)
        for (size_t y = 2; y < v.height() - 2; ++y)
        {
          unsigned char alpha = gil::get_color(v(x, y), gil::alpha_t());
          //if (alpha != 0)
          //{
            float const fAlpha = alpha / (float)DATA_TRAITS::maxChannelVal;
            gil::get_color(v(x, y), gil::red_t()) *= fAlpha;
            gil::get_color(v(x, y), gil::green_t()) *= fAlpha;
            gil::get_color(v(x, y), gil::blue_t()) *= fAlpha;

          //gil::get_color(v(x, y), gil::alpha_t()) = DATA_TRAITS::maxChannelVal;
          //}
        }
      */
    }
  }
}

#include "resource_style.hpp"

#include "glyph_cache.hpp"
#include "agg_traits.hpp"
#include "data_traits.hpp"

namespace yg
{
  GlyphStyle::GlyphStyle(m2::RectU const & texRect, int pipelineID, shared_ptr<GlyphInfo> const & gi)
    : ResourceStyle(EGlyphStyle, texRect, pipelineID), m_gi(gi)
  {}

  void GlyphStyle::render(void * dst)
  {
    shared_ptr<GlyphInfo> const & gi = m_gi;
    m2::RectU const & rect = m_texRect;

    DATA_TRAITS::view_t v = gil::interleaved_view(
          rect.SizeX(), rect.SizeY(),
          (DATA_TRAITS::pixel_t*)dst,
          sizeof(DATA_TRAITS::pixel_t) * rect.SizeX()
      );

    DATA_TRAITS::pixel_t pxTranslucent;
    gil::get_color(pxTranslucent, gil::red_t()) = gi->m_color.r / DATA_TRAITS::channelScaleFactor;
    gil::get_color(pxTranslucent, gil::green_t()) = gi->m_color.g / DATA_TRAITS::channelScaleFactor;
    gil::get_color(pxTranslucent, gil::blue_t()) = gi->m_color.b / DATA_TRAITS::channelScaleFactor;
    gil::get_color(pxTranslucent, gil::alpha_t()) = 0;

    for (size_t y = 0; y < 2; ++y)
      for (size_t x = 0; x < rect.SizeX(); ++x)
        v(x, y) = pxTranslucent;

    for (size_t y = rect.SizeY() - 2; y < rect.SizeY(); ++y)
      for (size_t x = 0; x < rect.SizeX(); ++x)
        v(x, y) = pxTranslucent;

    for (size_t y = 2; y < rect.SizeY() - 2; ++y)
    {
      v(0, y) = pxTranslucent;
      v(1, y) = pxTranslucent;
      v(rect.SizeX() - 2, y) = pxTranslucent;
      v(rect.SizeX() - 1, y) = pxTranslucent;
    }

    if ((gi->m_metrics.m_width != 0) && (gi->m_metrics.m_height != 0))
    {
      gil::gray8c_view_t srcView = gil::interleaved_view(
            gi->m_metrics.m_width,
            gi->m_metrics.m_height,
            (gil::gray8_pixel_t*)gi->m_bitmapData,
            gi->m_bitmapPitch
            );

/*        DATA_TRAITS::const_view_t srcView = gil::interleaved_view(
          gi->m_metrics.m_width,
          gi->m_metrics.m_height,
          (TDynamicTexture::pixel_t*)&gi->m_bitmap[0],
          gi->m_metrics.m_width * sizeof(TDynamicTexture::pixel_t)
          );*/

      DATA_TRAITS::pixel_t c;

      gil::get_color(c, gil::red_t()) = gi->m_color.r / DATA_TRAITS::channelScaleFactor;
      gil::get_color(c, gil::green_t()) = gi->m_color.g / DATA_TRAITS::channelScaleFactor;
      gil::get_color(c, gil::blue_t()) = gi->m_color.b / DATA_TRAITS::channelScaleFactor;
      gil::get_color(c, gil::alpha_t()) = gi->m_color.a / DATA_TRAITS::channelScaleFactor;

      for (size_t y = 2; y < rect.SizeY() - 2; ++y)
        for (size_t x = 2; x < rect.SizeX() - 2; ++x)
         {
           gil::get_color(c, gil::alpha_t()) = srcView(x - 2, y - 2) / DATA_TRAITS::channelScaleFactor;
           v(x, y) = c;
           gil::get_color(c, gil::alpha_t()) *= gi->m_color.a / 255.0f;
         }
    }
  }
}

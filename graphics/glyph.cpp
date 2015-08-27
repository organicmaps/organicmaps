#include "graphics/glyph.hpp"
#include "graphics/glyph_cache.hpp"
#include "graphics/agg_traits.hpp"

#include "graphics/opengl/data_traits.hpp"

namespace graphics
{
  Glyph::Info::Info()
    : Resource::Info(Resource::EGlyph)
  {}

  Glyph::Info::Info(GlyphKey const & key,
                    GlyphCache * cache)
    : Resource::Info(Resource::EGlyph),
      m_key(key),
      m_cache(cache)
  {
    m_metrics = m_cache->getGlyphMetrics(m_key);
  }

  Resource::Info const & Glyph::Info::cacheKey() const
  {
    return *this;
  }

  m2::PointU const Glyph::Info::resourceSize() const
  {
    return m2::PointU(m_metrics.m_width + 4,
                      m_metrics.m_height + 4);
  }

  Resource * Glyph::Info::createResource(m2::RectU const & texRect,
                                         uint8_t pipelineID) const
  {
    return new Glyph(*this,
                     texRect,
                     pipelineID);
  }

  bool Glyph::Info::lessThan(Resource::Info const * r) const
  {
    if (m_category != r->m_category)
      return m_category < r->m_category;

    Glyph::Info const * ri = static_cast<Glyph::Info const *>(r);

    if (m_key != ri->m_key)
      return m_key < ri->m_key;

    return false;
  }

  Glyph::Glyph(Info const & info,
               m2::RectU const & texRect,
               int pipelineID)
    : Resource(EGlyph,
               texRect,
               pipelineID),
      m_info(info)
  {
    m_bitmap = m_info.m_cache->getGlyphBitmap(m_info.m_key);
  }

  void Glyph::render(void * dst)
  {
    m2::RectU const & rect = m_texRect;

    DATA_TRAITS::view_t v = gil::interleaved_view(
          rect.SizeX(), rect.SizeY(),
          (DATA_TRAITS::pixel_t*)dst,
          sizeof(DATA_TRAITS::pixel_t) * rect.SizeX()
      );

    DATA_TRAITS::pixel_t pxTranslucent;

    gil::get_color(pxTranslucent, gil::red_t()) = m_info.m_key.m_color.r / DATA_TRAITS::channelScaleFactor;
    gil::get_color(pxTranslucent, gil::green_t()) = m_info.m_key.m_color.g / DATA_TRAITS::channelScaleFactor;
    gil::get_color(pxTranslucent, gil::blue_t()) = m_info.m_key.m_color.b / DATA_TRAITS::channelScaleFactor;
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

    if ((m_info.m_metrics.m_width != 0)
     && (m_info.m_metrics.m_height != 0))
    {
      gil::gray8c_view_t srcView = gil::interleaved_view(
            m_info.m_metrics.m_width,
            m_info.m_metrics.m_height,
            (gil::gray8_pixel_t*)&m_bitmap->m_data[0],
            m_bitmap->m_pitch
            );

      DATA_TRAITS::pixel_t c;

      gil::get_color(c, gil::red_t()) = m_info.m_key.m_color.r / DATA_TRAITS::channelScaleFactor;
      gil::get_color(c, gil::green_t()) = m_info.m_key.m_color.g / DATA_TRAITS::channelScaleFactor;
      gil::get_color(c, gil::blue_t()) = m_info.m_key.m_color.b / DATA_TRAITS::channelScaleFactor;
      gil::get_color(c, gil::alpha_t()) = m_info.m_key.m_color.a / DATA_TRAITS::channelScaleFactor;

      const float alpha = static_cast<float>(m_info.m_key.m_color.a) / DATA_TRAITS::maxChannelVal;

      for (size_t y = 2; y < rect.SizeY() - 2; ++y)
        for (size_t x = 2; x < rect.SizeX() - 2; ++x)
         {
           gil::get_color(c, gil::alpha_t()) = static_cast<uint8_t>(alpha * srcView(x - 2, y - 2)) / DATA_TRAITS::channelScaleFactor;
           v(x, y) = c;
         }
    }
  }

  Resource::Info const * Glyph::info() const
  {
    return &m_info;
  }
}

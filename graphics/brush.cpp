#include "graphics/brush.hpp"
#include "graphics/opengl/data_traits.hpp"

namespace graphics
{
  Brush::Info::Info()
    : Resource::Info(Resource::EBrush)
  {}

  Brush::Info::Info(Color const & color)
    : Resource::Info(Resource::EBrush),
      m_color(color)
  {}

  Resource::Info const & Brush::Info::cacheKey() const
  {
    return *this;
  }

  m2::PointU const Brush::Info::resourceSize() const
  {
    return m2::PointU(2, 2);
  }

  Resource * Brush::Info::createResource(m2::RectU const & texRect,
                                         uint8_t pipelineID) const
  {
    return new Brush(texRect,
                     pipelineID,
                     *this);
  }

  bool Brush::Info::lessThan(Resource::Info const * r) const
  {
    if (m_category != r->m_category)
      return m_category < r->m_category;

    Brush::Info const * br = static_cast<Brush::Info const*>(r);

    if (m_color != br->m_color)
      return m_color < br->m_color;

    return false;
  }

  Brush::Brush(m2::RectU const & texRect,
               uint8_t pipelineID,
               Info const & info)
    : Resource(EBrush, texRect, pipelineID),
      m_info(info)
  {
  }

  void Brush::render(void *dst)
  {
    graphics::Color c = m_info.m_color;
    m2::RectU const & r = m_texRect;

    DATA_TRAITS::pixel_t px;

    gil::get_color(px, gil::red_t()) = c.r / DATA_TRAITS::channelScaleFactor;
    gil::get_color(px, gil::green_t()) = c.g / DATA_TRAITS::channelScaleFactor;
    gil::get_color(px, gil::blue_t()) = c.b / DATA_TRAITS::channelScaleFactor;
    gil::get_color(px, gil::alpha_t()) = c.a / DATA_TRAITS::channelScaleFactor;

    DATA_TRAITS::view_t v = gil::interleaved_view(
        r.SizeX(), r.SizeY(),
        (DATA_TRAITS::pixel_t*)dst,
        sizeof(DATA_TRAITS::pixel_t) * r.SizeX()
    );

    for (size_t y = 0; y < r.SizeY(); ++y)
      for (size_t x = 0; x < r.SizeX(); ++x)
        v(x, y) = px;
  }

  Resource::Info const * Brush::info() const
  {
    return &m_info;
  }
}

#include "resource_style.hpp"

#include "data_traits.hpp"

namespace graphics
{
  ResourceStyle::ResourceStyle()
  {}

  ResourceStyle::ResourceStyle(
      m2::RectU const & texRect,
      int pipelineID
      ) : m_cat(EUnknownStyle),
      m_texRect(texRect),
      m_pipelineID(pipelineID)
  {}

  ResourceStyle::ResourceStyle(
      Category cat,
      m2::RectU const & texRect,
      int pipelineID)
    : m_cat(cat),
    m_texRect(texRect),
    m_pipelineID(pipelineID)
  {}

  ResourceStyle::~ResourceStyle()
  {}

  PointStyle::PointStyle(m2::RectU const & texRect, int pipelineID, string const & styleName)
    : ResourceStyle(EPointStyle, texRect, pipelineID), m_styleName(styleName)
  {}

  void PointStyle::render(void *dst)
  {}

  ColorStyle::ColorStyle(m2::RectU const & texRect, int pipelineID, graphics::Color const & c)
    : ResourceStyle(EColorStyle, texRect, pipelineID), m_c(c)
  {}

  void ColorStyle::render(void * dst)
  {
    graphics::Color c = m_c;
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
}

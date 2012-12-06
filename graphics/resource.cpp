#include "resource_style.hpp"

#include "opengl/data_traits.hpp"

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

  ImageStyle::ImageStyle(m2::RectU const & texRect,
                         int pipelineID,
                         ImageInfo const & ii)
    : ResourceStyle(EImageStyle, texRect, pipelineID),
      m_ii(ii)
  {}

  void ImageStyle::render(void * dst)
  {
    m2::RectU const & r = m_texRect;

    DATA_TRAITS::view_t dstV = gil::interleaved_view(
          r.SizeX(), r.SizeY(),
          (DATA_TRAITS::pixel_t*)dst,
          sizeof(DATA_TRAITS::pixel_t) * r.SizeX()
    );

    DATA_TRAITS::view_t srcV = gil::interleaved_view(
          r.SizeX() - 4, r.SizeY() - 4,
          (DATA_TRAITS::pixel_t*)m_ii.data(),
          sizeof(DATA_TRAITS::pixel_t) * (r.SizeX() - 4)
          );

    DATA_TRAITS::pixel_t borderPixel = DATA_TRAITS::createPixel(Color(255, 0, 0, 255));

    for (unsigned y = 0; y < 2; ++y)
    {
      dstV(0, y) = borderPixel;
      dstV(1, y) = borderPixel;
      dstV(r.SizeX() - 2, y) = borderPixel;
      dstV(r.SizeX() - 1, y) = borderPixel;
    }

    for (unsigned y = r.SizeY() - 2; y < r.SizeY(); ++y)
    {
      dstV(0, y) = borderPixel;
      dstV(1, y) = borderPixel;
      dstV(r.SizeX() - 2, y) = borderPixel;
      dstV(r.SizeX() - 1, y) = borderPixel;
    }

    gil::copy_pixels(srcV, gil::subimage_view(dstV, 2, 2, r.SizeX() - 4, r.SizeY() - 4));
  }
}

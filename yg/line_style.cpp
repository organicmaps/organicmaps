#include "resource_style.hpp"

#include "agg_traits.hpp"
#include "data_traits.hpp"

namespace yg
{
  LineStyle::LineStyle(bool isWrapped, m2::RectU const & texRect, int pipelineID, yg::PenInfo const & penInfo) :
    ResourceStyle(ELineStyle, texRect, pipelineID),
    m_isWrapped(isWrapped),
    m_isSolid(penInfo.m_isSolid),
    m_penInfo(penInfo)
  {
    if (m_isSolid)
    {
      m_borderColorPixel = m_centerColorPixel = m2::PointU(texRect.minX() + 1, texRect.minY() + 1);
    }
    else
    {
      double firstDashOffset = penInfo.firstDashOffset();
      m_centerColorPixel = m2::PointU(static_cast<uint32_t>(firstDashOffset + texRect.minX() + 3),
                                      static_cast<uint32_t>(texRect.minY() + texRect.SizeY() / 2.0));
      m_borderColorPixel = m2::PointU(static_cast<uint32_t>(firstDashOffset + texRect.minX() + 3),
                                    static_cast<uint32_t>(texRect.minY() + 1));
    }
  }

  double LineStyle::geometryTileLen() const
  {
    return m_texRect.SizeX() - 2;
  }

  double LineStyle::geometryTileWidth() const
  {
    return m_texRect.SizeY() - 2;
  }

  double LineStyle::rawTileLen() const
  {
    return m_texRect.SizeX() - 4;
  }

  double LineStyle::rawTileWidth() const
  {
    return m_texRect.SizeY() - 4;
  }

  void LineStyle::render(void * dst)
  {
    yg::PenInfo const & penInfo = m_penInfo;
    m2::RectU const & rect = m_texRect;

    DATA_TRAITS::view_t v = gil::interleaved_view(rect.SizeX(),
                                                      rect.SizeY(),
                                                      (DATA_TRAITS::pixel_t*)dst,
                                                      sizeof(DATA_TRAITS::pixel_t) * rect.SizeX());

    yg::Color penInfoColor = penInfo.m_color;

    penInfoColor /= DATA_TRAITS::channelScaleFactor;

    DATA_TRAITS::pixel_t penColorTranslucent;

    gil::get_color(penColorTranslucent, gil::red_t()) = penInfoColor.r;
    gil::get_color(penColorTranslucent, gil::green_t()) = penInfoColor.g;
    gil::get_color(penColorTranslucent, gil::blue_t()) = penInfoColor.b;
    gil::get_color(penColorTranslucent, gil::alpha_t()) = 0;

    DATA_TRAITS::pixel_t penColor = penColorTranslucent;
    gil::get_color(penColor, gil::alpha_t()) = penInfoColor.a;

    if (penInfo.m_isSolid)
    {
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

      gil::fill_pixels(v, penColorTranslucent);

      agg::scanline_u8 s;
      agg::rasterizer_scanline_aa<> rasterizer;
      if (penInfo.m_w > 2)
      {
        agg::ellipse ell;
        float r = ceil(penInfo.m_w) / 2.0;
        ell.init(r + 2, r + 2, r, r, 100);
        rasterizer.add_path(ell);

        agg::render_scanlines_aa_solid(rasterizer,
                                       s,
                                       rbase,
                                       agg::rgba8(penInfo.m_color.r,
                                                  penInfo.m_color.g,
                                                  penInfo.m_color.b,
                                                  penInfo.m_color.a));

        uint32_t ri = static_cast<uint32_t>(r);

        /// pixels that are used to texture inner part of the line should be fully opaque
        v(2 + ri - 1, 2) = penColor;
        v(2 + ri    , 2) = penColor;
        v(2 + ri - 1, 2 + ri * 2 - 1) = penColor;
        v(2 + ri    , 2 + ri * 2 - 1) = penColor;

        /// in non-transparent areas - premultiply color value with alpha and make it opaque
        for (size_t x = 2; x < v.width() - 2; ++x)
          for (size_t y = 2; y < v.height() - 2; ++y)
          {
            unsigned char alpha = gil::get_color(v(x, y), gil::alpha_t());
            if (alpha != 0)
            {
              v(x, y) = penColor;
//                if (m_fillAlpha)
//                  gil::get_color(v(x, y), gil::alpha_t()) = alpha;
            }
          }
      }
      else
      {
        gil::fill_pixels(
            gil::subimage_view(v, 2, 2, rect.SizeX() - 4, rect.SizeY() - 4),
            penColor
            );
      }
    }
    else
    {
      /// First two and last two rows of a pattern are filled
      /// with a penColorTranslucent pixel for the antialiasing.
      for (size_t y = 0; y < 2; ++y)
        for (size_t x = 0; x < rect.SizeX(); ++x)
          v(x, y) = penColorTranslucent;

      for (size_t y = rect.SizeY() - 2; y < rect.SizeY(); ++y)
        for (size_t x = 0; x < rect.SizeX(); ++x)
          v(x, y) = penColorTranslucent;

      /// first and last two pixels filled with penColorTranslucent
      for (size_t y = 2; y < rect.SizeY() - 2; ++y)
      {
        v(0, y) = penColorTranslucent;
        v(1, y) = penColorTranslucent;
        v(rect.SizeX() - 2, y) = penColorTranslucent;
        v(rect.SizeX() - 1, y) = penColorTranslucent;
      }

      /// draw pattern
      for (size_t y = 2; y < rect.SizeY() - 2; ++y)
      {
        double curLen = 0;

        DATA_TRAITS::pixel_t px = penColor;

        /// In general case this code is incorrect.
        /// TODO : Make a pattern start and end with a dash.
        uint32_t curLenI = static_cast<uint32_t>(curLen);

        v(curLenI, y) = px;
        v(curLenI + 1, y) = px;

        for (size_t i = 0; i < penInfo.m_pat.size(); ++i)
        {
          for (size_t j = 0; j < penInfo.m_pat[i]; ++j)
          {
            uint32_t val = (i + 1) % 2;

            if (val == 0)
              gil::get_color(px, gil::alpha_t()) = 0;
            else
              gil::get_color(px, gil::alpha_t()) = penInfoColor.a;

            v(curLenI + j + 2, y) = px;
          }

          v(static_cast<uint32_t>(curLen + 2 + penInfo.m_pat[i]), y) = px;
          v(static_cast<uint32_t>(curLen + 2 + penInfo.m_pat[i] + 1), y) = px;

          curLen += penInfo.m_pat[i];

          curLenI = static_cast<uint32_t>(curLen);
        }
      }
    }
  }
}

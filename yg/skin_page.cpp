#include <agg_pixfmt_rgba.h>
#include <agg_pixfmt_rgb_packed.h>
#include <agg_renderer_scanline.h>
#include <agg_rasterizer_scanline_aa.h>
#include <agg_scanline_u.h>
#include <agg_ellipse.h>

#include "texture.hpp"
#include "data_formats.hpp"
#include "skin_page.hpp"
#include "resource_style.hpp"
#include "resource_manager.hpp"
#include "internal/opengl.hpp"

#include "../base/logging.hpp"

#include "../std/bind.hpp"
#include "../std/numeric.hpp"

template <typename Traits>
struct AggTraits
{
};

template <>
struct AggTraits<yg::RGBA8Traits>
{
  typedef agg::pixfmt_rgba32 pixfmt_t;
};

template <>
struct AggTraits<yg::RGBA4Traits>
{
  typedef agg::pixfmt_rgb4444 pixfmt_t;
};

namespace yg
{
  typedef gl::Texture<DATA_TRAITS, true> TDynamicTexture;

  GlyphUploadCmd::GlyphUploadCmd(){}
  GlyphUploadCmd::GlyphUploadCmd(shared_ptr<GlyphInfo> const & glyphInfo,
                                 m2::RectU const & rect)
                               : m_glyphInfo(glyphInfo),
                                 m_rect(rect){}

  ColorUploadCmd::ColorUploadCmd(){}
  ColorUploadCmd::ColorUploadCmd(yg::Color const & color,
                                 m2::RectU const & rect)
                               : m_color(color),
                                 m_rect(rect){}

  PenUploadCmd::PenUploadCmd(){}
  PenUploadCmd::PenUploadCmd(yg::PenInfo const & penInfo,
                             m2::RectU const & rect)
                           : m_penInfo(penInfo),
                             m_rect(rect){}

  CircleUploadCmd::CircleUploadCmd(){}
  CircleUploadCmd::CircleUploadCmd(yg::CircleInfo const & circleInfo,
                                   m2::RectU const & rect)
                                 : m_circleInfo(circleInfo),
                                 m_rect(rect){}

  ResourceStyle * FontInfo::fromID(uint32_t id, bool isMask) const
  {
    TChars::const_iterator it = m_chars.find(id);
    if (it == m_chars.end())
    {
      if (m_invalidChar.first == 0)
      {
        it = m_chars.find(65533);

        if (it == m_chars.end())
          it = m_chars.find(32);
        else
          LOG(LINFO, ("initialized invalidChar from 65533"));

        m_invalidChar = pair<ResourceStyle*, ResourceStyle*>(it->second.first.get(), it->second.second.get());
      }

      if (isMask)
        return m_invalidChar.second;
      else
        return m_invalidChar.first;
    }
    else
      if (isMask)
        return it->second.second.get();
      else
        return it->second.first.get();
  }

  SkinPage::SkinPage(shared_ptr<ResourceManager> const & resourceManager,
                     char const * name,
                     uint8_t pageID,
                     bool fillAlpha)
                   : m_texture(resourceManager->getTexture(name)),
                     m_isDynamic(false),
                     m_pageID(pageID),
                     m_fillAlpha(fillAlpha)
  {
    m_packer = m2::Packer(m_texture->width(), m_texture->height(), 0x00FFFFFF - 1);
  }


  SkinPage::SkinPage(shared_ptr<ResourceManager> const & resourceManager,
                     uint8_t pageID,
                     bool fillAlpha)
    : m_resourceManager(resourceManager),
      m_isDynamic(true),
      m_pageID(pageID),
      m_fillAlpha(fillAlpha)
  {
    m_packer = m2::Packer(m_resourceManager->textureWidth(),
                          m_resourceManager->textureHeight(),
                          0x00FFFFFF - 1);
    /// clear handles will be called only upon handles overflow,
    /// as the texture overflow is processed separately
    m_packer.addOverflowFn(bind(&SkinPage::clearHandles, this), 0);
  }

  void SkinPage::clearHandles()
  {
    clearPenInfoHandles();
    clearColorHandles();
    clearFontHandles();
    clearCircleInfoHandles();

    m_packer.reset();
  }

  void SkinPage::clearColorHandles()
  {
    for (TColorMap::const_iterator it = m_colorMap.begin(); it != m_colorMap.end(); ++it)
      m_styles.erase(it->second);

    m_colorMap.clear();
  }

  void SkinPage::clearPenInfoHandles()
  {
    for (TPenInfoMap::const_iterator it = m_penInfoMap.begin(); it != m_penInfoMap.end(); ++it)
      m_styles.erase(it->second);

    m_penInfoMap.clear();
  }

  void SkinPage::clearCircleInfoHandles()
  {
    for (TCircleInfoMap::const_iterator it = m_circleInfoMap.begin(); it != m_circleInfoMap.end(); ++it)
      m_styles.erase(it->second);

    m_circleInfoMap.clear();
  }

  void SkinPage::clearFontHandles()
  {
    for (TGlyphMap::const_iterator it = m_glyphMap.begin(); it != m_glyphMap.end(); ++it)
      m_styles.erase(it->second);

    m_glyphMap.clear();
  }

  uint32_t SkinPage::findColor(yg::Color const & c) const
  {
    TColorMap::const_iterator it = m_colorMap.find(c);
    if (it == m_colorMap.end())
      return m_packer.invalidHandle();
    else
      return it->second;
  }

  uint32_t SkinPage::mapColor(yg::Color const & c)
  {
    uint32_t foundHandle = findColor(c);
    if (foundHandle != m_packer.invalidHandle())
      return foundHandle;

    m2::Packer::handle_t h = m_packer.pack(2, 2);

    m2::RectU texRect = m_packer.find(h).second;

    m_colorUploadCommands.push_back(ColorUploadCmd(c, texRect));
    m_colorMap[c] = h;

    m_styles[h] = shared_ptr<ResourceStyle>(new GenericStyle(texRect, m_pageID));

    return h;
  }

  bool SkinPage::hasRoom(Color const & ) const
  {
    return m_packer.hasRoom(2, 2);
  }

  uint32_t SkinPage::findSymbol(char const * symbolName) const
  {
    TPointNameMap::const_iterator it = m_pointNameMap.find(symbolName);
    if (it == m_pointNameMap.end())
      return m_packer.invalidHandle();
    else
      return it->second;
  }

  uint32_t SkinPage::findGlyph(GlyphKey const & g, bool isFixedFont) const
  {
    if (isFixedFont)
    {
      TStyles::const_iterator styleIt = m_styles.find(g.toUInt32());
      if (styleIt != m_styles.end())
        return g.toUInt32();
      TFonts::const_iterator fontIt = m_fonts.begin();
      int lastFontSize = 0;
      if (!m_fonts.empty())
        lastFontSize = m_fonts[0].m_fontSize;

      for (TFonts::const_iterator it = m_fonts.begin(); it != m_fonts.end(); ++it)
        if ((lastFontSize < g.m_fontSize) && (g.m_fontSize >= it->m_fontSize))
          fontIt = it;
        else
          lastFontSize = it->m_fontSize;

      if (fontIt != m_fonts.end())
      {
        FontInfo::TChars::const_iterator charIt = fontIt->m_chars.find(g.m_id);
        if (charIt != fontIt->m_chars.end())
        {
          if (g.m_isMask)
            const_cast<TStyles&>(m_styles)[g.toUInt32()] = charIt->second.second;
          else
            const_cast<TStyles&>(m_styles)[g.toUInt32()]= charIt->second.first;
          return g.toUInt32();
        }
      }

      return m_packer.invalidHandle();
    }

    TGlyphMap::const_iterator it = m_glyphMap.find(g);
    if (it == m_glyphMap.end())
      return m_packer.invalidHandle();
    else
      return it->second;
  }

  uint32_t SkinPage::mapGlyph(yg::GlyphKey const & g)
  {
    uint32_t foundHandle = findGlyph(g, false);
    if (foundHandle != m_packer.invalidHandle())
      return foundHandle;

    shared_ptr<GlyphInfo> gi = m_resourceManager->getGlyph(g);

    m2::Packer::handle_t handle = m_packer.pack(gi->m_metrics.m_width + 4,
                                                gi->m_metrics.m_height + 4);

    m2::RectU texRect = m_packer.find(handle).second;
    m_glyphUploadCommands.push_back(GlyphUploadCmd(gi, texRect));
    m_glyphMap[g] = handle;

    m_styles[handle] = boost::shared_ptr<ResourceStyle>(
        new CharStyle(texRect,
                      m_pageID,
                      gi->m_metrics.m_xOffset,
                      gi->m_metrics.m_yOffset,
                      gi->m_metrics.m_xAdvance));

    return m_glyphMap[g];
  }

  bool SkinPage::hasRoom(GlyphKey const & gk) const
  {
    shared_ptr<GlyphInfo> gi = m_resourceManager->getGlyph(gk);
    return m_packer.hasRoom(gi->m_metrics.m_width + 4, gi->m_metrics.m_height + 4);
  }

  uint32_t SkinPage::findCircleInfo(CircleInfo const & circleInfo) const
  {
    TCircleInfoMap::const_iterator it = m_circleInfoMap.find(circleInfo);
    if (it == m_circleInfoMap.end())
      return m_packer.invalidHandle();
    else
      return it->second;
  }

  uint32_t SkinPage::mapCircleInfo(CircleInfo const & circleInfo)
  {
    uint32_t foundHandle = findCircleInfo(circleInfo);

    if (foundHandle != m_packer.invalidHandle())
      return foundHandle;

    unsigned r = circleInfo.m_isOutlined ? circleInfo.m_radius + circleInfo.m_outlineWidth : circleInfo.m_radius;

    m2::Packer::handle_t handle = m_packer.pack(
        r * 2 + 4,
        r * 2 + 4);

    m2::RectU texRect = m_packer.find(handle).second;
    m_circleUploadCommands.push_back(CircleUploadCmd(circleInfo, texRect));
    m_circleInfoMap[circleInfo] = handle;

    m_styles[handle] = shared_ptr<ResourceStyle>(new GenericStyle(texRect, m_pageID) );

    return m_circleInfoMap[circleInfo];
  }

  bool SkinPage::hasRoom(CircleInfo const & circleInfo) const
  {
    unsigned r = circleInfo.m_isOutlined ? circleInfo.m_radius + circleInfo.m_outlineWidth : circleInfo.m_radius;
    return m_packer.hasRoom(r * 2 + 4,
                            r * 2 + 4);
  }

  uint32_t SkinPage::findPenInfo(PenInfo const & penInfo) const
  {
    TPenInfoMap::const_iterator it = m_penInfoMap.find(penInfo);
    if (it == m_penInfoMap.end())
      return m_packer.invalidHandle();
    else
      return it->second;
  }

  uint32_t SkinPage::mapPenInfo(PenInfo const & penInfo)
  {
    uint32_t foundHandle = findPenInfo(penInfo);

    if (foundHandle != m_packer.invalidHandle())
      return foundHandle;

    m2::PointU p = penInfo.patternSize();

    m2::Packer::handle_t handle = m_packer.pack(p.x, p.y);
    m2::RectU texRect = m_packer.find(handle).second;
    m_penUploadCommands.push_back(PenUploadCmd(penInfo, texRect));
    m_penInfoMap[penInfo] = handle;

    m_styles[handle] = boost::shared_ptr<ResourceStyle>(
        new LineStyle(false,
                      texRect,
                      m_pageID,
                      penInfo));

    return m_penInfoMap[penInfo];
  }

  bool SkinPage::hasRoom(const PenInfo &penInfo) const
  {
    m2::PointU p = penInfo.patternSize();
    return m_packer.hasRoom(p.x, p.y);
  }

  bool SkinPage::isDynamic() const
  {
    return m_isDynamic;
  }

  void SkinPage::uploadPenInfo()
  {
    for (size_t i = 0; i < m_penUploadCommands.size(); ++i)
    {
      yg::PenInfo const & penInfo = m_penUploadCommands[i].m_penInfo;
      m2::RectU const & rect = m_penUploadCommands[i].m_rect;

      TDynamicTexture * dynTexture = static_cast<TDynamicTexture*>(m_texture.get());

      TDynamicTexture::view_t v = dynTexture->view(rect.SizeX(), rect.SizeY());

      yg::Color penInfoColor = penInfo.m_color;

      penInfoColor /= TDynamicTexture::channelScaleFactor;

      TDynamicTexture::pixel_t penColorTranslucent;

      gil::get_color(penColorTranslucent, gil::red_t()) = penInfoColor.r;
      gil::get_color(penColorTranslucent, gil::green_t()) = penInfoColor.g;
      gil::get_color(penColorTranslucent, gil::blue_t()) = penInfoColor.b;
      gil::get_color(penColorTranslucent, gil::alpha_t()) = 0;

      TDynamicTexture::pixel_t penColor = penColorTranslucent;
      gil::get_color(penColor, gil::alpha_t()) = penInfoColor.a;

      if (penInfo.m_isSolid)
      {
        /// draw circle
        agg::rendering_buffer buf(
            (unsigned char *)&v(0, 0),
            rect.SizeX(),
            rect.SizeY(),
            rect.SizeX() * sizeof(TDynamicTexture::pixel_t)
            );

        typedef AggTraits<TDynamicTexture::traits_t>::pixfmt_t agg_pixfmt_t;

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

          /// pixels that are used to texture inner part of the line should be fully opaque
          v(2 + r - 1, 2) = penColor;
          v(2 + r    , 2) = penColor;
          v(2 + r - 1, 2 + r * 2 - 1) = penColor;
          v(2 + r    , 2 + r * 2 - 1) = penColor;

          /// in non-transparent areas - premultiply color value with alpha and make it opaque
          for (size_t x = 2; x < v.width() - 2; ++x)
            for (size_t y = 2; y < v.height() - 2; ++y)
            {
              unsigned char alpha = gil::get_color(v(x, y), gil::alpha_t());
              if (alpha != 0)
              {
                v(x, y) = penColor;
                if (m_fillAlpha)
                  gil::get_color(v(x, y), gil::alpha_t()) = alpha;
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

          TDynamicTexture::pixel_t px = penColor;

          /// In general case this code is incorrect.
          /// TODO : Make a pattern start and end with a dash.
          v(curLen, y) = px;
          v(curLen + 1, y) = px;

          for (size_t i = 0; i < penInfo.m_pat.size(); ++i)
          {
            for (size_t j = 0; j < penInfo.m_pat[i]; ++j)
            {
              uint32_t val = (i + 1) % 2;

              if (val == 0)
                gil::get_color(px, gil::alpha_t()) = 0;
              else
                gil::get_color(px, gil::alpha_t()) = penInfoColor.a;

              v(curLen + j + 2, y) = px;
            }

            v(curLen + 2 + penInfo.m_pat[i], y) = px;
            v(curLen + 2 + penInfo.m_pat[i] + 1, y) = px;

            curLen += penInfo.m_pat[i];
          }
        }
      }

      dynTexture->upload(&v(0, 0), rect);

    }
    m_penUploadCommands.clear();
  }

  void SkinPage::uploadCircleInfo()
  {
    for (size_t i = 0; i < m_circleUploadCommands.size(); ++i)
    {
      yg::CircleInfo & circleInfo = m_circleUploadCommands[i].m_circleInfo;
      m2::RectU const & rect = m_circleUploadCommands[i].m_rect;

      TDynamicTexture * dynTexture = static_cast<TDynamicTexture*>(m_texture.get());

      TDynamicTexture::view_t v = dynTexture->view(rect.SizeX(), rect.SizeY());

      agg::rgba8 aggColor(circleInfo.m_color.r,
                          circleInfo.m_color.g,
                          circleInfo.m_color.b,
                          circleInfo.m_color.a);

      agg::rgba8 aggOutlineColor(circleInfo.m_outlineColor.r,
                                 circleInfo.m_outlineColor.g,
                                 circleInfo.m_outlineColor.b,
                                 circleInfo.m_outlineColor.a);

      circleInfo.m_color /= TDynamicTexture::channelScaleFactor;

      TDynamicTexture::pixel_t gilColorTranslucent;

      gil::get_color(gilColorTranslucent, gil::red_t()) = circleInfo.m_color.r;
      gil::get_color(gilColorTranslucent, gil::green_t()) = circleInfo.m_color.g;
      gil::get_color(gilColorTranslucent, gil::blue_t()) = circleInfo.m_color.b;
      gil::get_color(gilColorTranslucent, gil::alpha_t()) = 0;

      circleInfo.m_outlineColor /= TDynamicTexture::channelScaleFactor;

      TDynamicTexture::pixel_t gilOutlineColorTranslucent;

      gil::get_color(gilOutlineColorTranslucent, gil::red_t()) = circleInfo.m_outlineColor.r;
      gil::get_color(gilOutlineColorTranslucent, gil::green_t()) = circleInfo.m_outlineColor.g;
      gil::get_color(gilOutlineColorTranslucent, gil::blue_t()) = circleInfo.m_outlineColor.b;
      gil::get_color(gilOutlineColorTranslucent, gil::alpha_t()) = 0;

      TDynamicTexture::pixel_t gilColor = gilColorTranslucent;
      gil::get_color(gilColor, gil::alpha_t()) = circleInfo.m_color.a;

      TDynamicTexture::pixel_t gilOutlineColor = gilOutlineColorTranslucent;
      gil::get_color(gilOutlineColor, gil::alpha_t()) = circleInfo.m_outlineColor.a;

      /// draw circle
      agg::rendering_buffer buf(
          (unsigned char *)&v(0, 0),
          rect.SizeX(),
          rect.SizeY(),
          rect.SizeX() * sizeof(TDynamicTexture::pixel_t)
          );

      typedef AggTraits<TDynamicTexture::traits_t>::pixfmt_t agg_pixfmt_t;

      agg_pixfmt_t pixfmt(buf);
      agg::renderer_base<agg_pixfmt_t> rbase(pixfmt);

      if (circleInfo.m_isOutlined)
        gil::fill_pixels(v, gilOutlineColorTranslucent);
      else
        gil::fill_pixels(v, gilColorTranslucent);

      m2::PointD center(circleInfo.m_radius + 2, circleInfo.m_radius + 2);

      if (circleInfo.m_isOutlined)
        center += m2::PointD(circleInfo.m_outlineWidth, circleInfo.m_outlineWidth);

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

      TDynamicTexture::pixel_t px = circleInfo.m_isOutlined ? gilOutlineColor : gilColor;

      /// making alpha channel opaque
/*      for (size_t x = 2; x < v.width() - 2; ++x)
        for (size_t y = 2; y < v.height() - 2; ++y)
        {
          unsigned char alpha = gil::get_color(v(x, y), gil::alpha_t());

          float fAlpha = alpha / (float)TDynamicTexture::maxChannelVal;

          if (alpha != 0)
          {
            gil::get_color(v(x, y), gil::red_t()) *= fAlpha;
            gil::get_color(v(x, y), gil::green_t()) *= fAlpha;
            gil::get_color(v(x, y), gil::blue_t()) *= fAlpha;

            gil::get_color(v(x, y), gil::alpha_t()) = TDynamicTexture::maxChannelVal;
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
/*        for (size_t x = 2; x < v.width() - 2; ++x)
          for (size_t y = 2; y < v.height() - 2; ++y)
          {
            unsigned char alpha = gil::get_color(v(x, y), gil::alpha_t());
            float fAlpha = alpha / (float)TDynamicTexture::maxChannelVal;
//            if (alpha != 0)
//            {
              gil::get_color(v(x, y), gil::red_t()) *= fAlpha;
              gil::get_color(v(x, y), gil::green_t()) *= fAlpha;
              gil::get_color(v(x, y), gil::blue_t()) *= fAlpha;

//              gil::get_color(v(x, y), gil::alpha_t()) = TDynamicTexture::maxChannelVal;
//            }
          }*/
      }

      dynTexture->upload(&v(0, 0), rect);
    }

    m_circleUploadCommands.clear();
  }

  void SkinPage::uploadGlyphs()
  {
    for (size_t i = 0; i < m_glyphUploadCommands.size(); ++i)
    {
      shared_ptr<GlyphInfo> const & gi = m_glyphUploadCommands[i].m_glyphInfo;
      m2::RectU const & rect = m_glyphUploadCommands[i].m_rect;

      TDynamicTexture * dynTexture = static_cast<TDynamicTexture*>(m_texture.get());

      TDynamicTexture::view_t v = dynTexture->view(rect.SizeX(), rect.SizeY());

      TDynamicTexture::pixel_t pxTranslucent;
      gil::get_color(pxTranslucent, gil::red_t()) = gi->m_color.r / TDynamicTexture::channelScaleFactor;
      gil::get_color(pxTranslucent, gil::green_t()) = gi->m_color.g / TDynamicTexture::channelScaleFactor;
      gil::get_color(pxTranslucent, gil::blue_t()) = gi->m_color.b / TDynamicTexture::channelScaleFactor;
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
        TDynamicTexture::const_view_t srcView = gil::interleaved_view(
            gi->m_metrics.m_width,
            gi->m_metrics.m_height,
            (TDynamicTexture::pixel_t*)&gi->m_bitmap[0],
            gi->m_metrics.m_width * sizeof(TDynamicTexture::pixel_t)
            );

        for (size_t y = 2; y < rect.SizeY() - 2; ++y)
          for (size_t x = 2; x < rect.SizeX() - 2; ++x)
           v(x, y) = srcView(x - 2, y - 2);
      }

      dynTexture->upload(&v(0, 0), rect);
    }

    m_glyphUploadCommands.clear();
  }

  void SkinPage::uploadColors()
  {
    for (size_t i = 0; i < m_colorUploadCommands.size(); ++i)
    {
      yg::Color c = m_colorUploadCommands[i].m_color;
      m2::RectU const & r = m_colorUploadCommands[i].m_rect;

      TDynamicTexture::pixel_t px;

      gil::get_color(px, gil::red_t()) = c.r / TDynamicTexture::channelScaleFactor;
      gil::get_color(px, gil::green_t()) = c.g / TDynamicTexture::channelScaleFactor;
      gil::get_color(px, gil::blue_t()) = c.b / TDynamicTexture::channelScaleFactor;
      gil::get_color(px, gil::alpha_t()) = c.a / TDynamicTexture::channelScaleFactor;

      TDynamicTexture * dynTexture = static_cast<TDynamicTexture*>(m_texture.get());

      TDynamicTexture::view_t v = dynTexture->view(r.SizeX(), r.SizeY());

      for (size_t y = 0; y < r.SizeY(); ++y)
        for (size_t x = 0; x < r.SizeX(); ++x)
          v(x, y) = px;

      dynTexture->upload(&v(0, 0), r);
    }
    m_colorUploadCommands.clear();
  }

  bool SkinPage::hasData()
  {
    return ((!m_colorUploadCommands.empty())
        || (!m_penUploadCommands.empty())
        || (!m_glyphUploadCommands.empty())
        || (!m_circleUploadCommands.empty()));
  }

  void SkinPage::checkTexture() const
  {
    if ((m_isDynamic) && (m_texture == 0))
      reserveTexture();
  }

  void SkinPage::uploadData()
  {
    if ((m_isDynamic) && (hasData()))
    {
      checkTexture();
      static_cast<gl::ManagedTexture*>(m_texture.get())->lock();
      uploadColors();
      uploadPenInfo();
      uploadGlyphs();
      uploadCircleInfo();
      static_cast<gl::ManagedTexture*>(m_texture.get())->unlock();
    }
  }

  ResourceStyle * SkinPage::fromID(uint32_t idx) const
  {
    TStyles::const_iterator it = m_styles.find(idx);

    if (it == m_styles.end())
      return 0;
    else
      return it->second.get();
  }

  void SkinPage::addOverflowFn(overflowFn fn, int priority)
  {
    m_packer.addOverflowFn(fn, priority);
  }

  shared_ptr<gl::BaseTexture> const & SkinPage::texture() const
  {
    checkTexture();
    return m_texture;
  }

  void SkinPage::freeTexture()
  {
    if (m_texture)
      m_resourceManager->freeTexture(m_texture);
    m_texture.reset();
  }

  void SkinPage::reserveTexture() const
  {
    m_texture = m_resourceManager->reserveTexture();
  }
}

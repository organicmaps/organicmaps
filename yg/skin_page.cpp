#include "../base/SRC_FIRST.hpp"

#include "data_formats.hpp"
#include "skin_page.hpp"
#include "texture.hpp"
#include "resource_style.hpp"
#include "resource_manager.hpp"
#include "internal/opengl.hpp"

#include "../base/logging.hpp"

#include "../std/bind.hpp"
#include "../std/numeric.hpp"

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
                     uint8_t pageID)
                   : m_texture(resourceManager->getTexture(name)),
                     m_isDynamic(false),
                     m_pageID(pageID)
  {
    m_packer = m2::Packer(m_texture->width(), m_texture->height(), 0x00FFFFFF - 1);
  }


  SkinPage::SkinPage(shared_ptr<ResourceManager> const & resourceManager,
                     uint8_t pageID)
    : m_texture(resourceManager->reserveTexture()),
      m_resourceManager(resourceManager),
      m_isDynamic(true),
      m_pageID(pageID)
  {
    m_packer = m2::Packer(m_texture->width(), m_texture->height(), 0x00FFFFFF - 1),
    /// clear handles will be called only upon handles overflow,
    /// as the texture overflow is processed separately
    m_packer.addOverflowFn(bind(&SkinPage::clearHandles, this), 0);
  }

  void SkinPage::clearHandles()
  {
    clearPenInfoHandles();
    clearColorHandles();
    clearFontHandles();

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

    if (penInfo.m_isSolid)
    {
      uint32_t colorHandle = mapColor(penInfo.m_color);
      m2::RectU texRect = m_packer.find(colorHandle).second;
      m_styles[colorHandle] = boost::shared_ptr<ResourceStyle>(
          new LineStyle(false,
                        texRect,
                        m_pageID,
                        penInfo));
      m_penInfoMap[penInfo] = colorHandle;
    }
    else
    {
      uint32_t len = static_cast<uint32_t>(accumulate(penInfo.m_pat.begin(), penInfo.m_pat.end(), 0.0));

      m2::Packer::handle_t handle = m_packer.pack(len + 4, penInfo.m_w + 4);
      m2::RectU texRect = m_packer.find(handle).second;
      m_penUploadCommands.push_back(PenUploadCmd(penInfo, texRect));
      m_penInfoMap[penInfo] = handle;

      m_styles[handle] = boost::shared_ptr<ResourceStyle>(
          new LineStyle(false,
                        texRect,
                        m_pageID,
                        penInfo));
    }

    return m_penInfoMap[penInfo];
  }

  bool SkinPage::hasRoom(const PenInfo &penInfo) const
  {
    if (penInfo.m_isSolid)
      return hasRoom(penInfo.m_color);
    else
    {
      uint32_t len = static_cast<uint32_t>(accumulate(penInfo.m_pat.begin(), penInfo.m_pat.end(), 0.0));
      return m_packer.hasRoom(len + 4, penInfo.m_w + 4);
    }
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

      penInfoColor.r /= TDynamicTexture::channelScaleFactor;
      penInfoColor.g /= TDynamicTexture::channelScaleFactor;
      penInfoColor.b /= TDynamicTexture::channelScaleFactor;
      penInfoColor.a /= TDynamicTexture::channelScaleFactor;

      TDynamicTexture::pixel_t penColorTranslucent;

      gil::get_color(penColorTranslucent, gil::red_t()) = penInfoColor.r;
      gil::get_color(penColorTranslucent, gil::green_t()) = penInfoColor.g;
      gil::get_color(penColorTranslucent, gil::blue_t()) = penInfoColor.b;
      gil::get_color(penColorTranslucent, gil::alpha_t()) = 0;

      TDynamicTexture::pixel_t penColor = penColorTranslucent;
      gil::get_color(penColor, gil::alpha_t()) = penInfoColor.a;

      /// First two and last two rows of a pattern are filled
      /// with a penColorTranslucent pixel for the antialiasing.
      for (size_t y = 0; y < 2; ++y)
        for (size_t x = 0; x < rect.SizeX(); ++x)
          v(x, y) = penColorTranslucent;

      for (size_t y = rect.SizeY() - 2; y < rect.SizeY(); ++y)
        for (size_t x = 0; x < rect.SizeX(); ++x)
          v(x, y) = penColorTranslucent;

      for (size_t y = 2; y < rect.SizeY() - 2; ++y)
      {
        v(0, y) = penColor;
        v(1, y) = penColor;
        v(rect.SizeX() - 2, y) = penColor;
        v(rect.SizeX() - 1, y) = penColor;
      }
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

      dynTexture->upload(&v(0, 0), rect);
    }
    m_penUploadCommands.clear();
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

  void SkinPage::uploadData()
  {
    if (m_isDynamic)
    {
      static_cast<gl::ManagedTexture*>(m_texture.get())->lock();
      uploadColors();
      uploadPenInfo();
      uploadGlyphs();
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
    return m_texture;
  }

  void SkinPage::freeTexture()
  {
    m_resourceManager->freeTexture(m_texture);
  }

  void SkinPage::reserveTexture()
  {
    m_texture = m_resourceManager->reserveTexture();
  }
}

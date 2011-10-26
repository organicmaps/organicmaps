#include "skin_page.hpp"

#include "texture.hpp"
#include "data_formats.hpp"
#include "resource_style.hpp"
#include "resource_manager.hpp"
#include "internal/opengl.hpp"

#include "../base/logging.hpp"

#include "../std/bind.hpp"
#include "../std/numeric.hpp"

namespace yg
{
  typedef gl::Texture<DATA_TRAITS, true> TDynamicTexture;

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

  SkinPage::SkinPage()
    : m_usage(EStaticUsage),
      m_pipelineID(0)
  {}


  SkinPage::SkinPage(shared_ptr<ResourceManager> const & resourceManager,
                     char const * name,
                     uint8_t pipelineID)
                   : m_texture(resourceManager->getTexture(name)),
                     m_usage(EStaticUsage),
                     m_pipelineID(pipelineID)
  {
    m_packer = m2::Packer(m_texture->width(), m_texture->height(), 0x00FFFFFF - 1);
  }

  SkinPage::SkinPage(shared_ptr<ResourceManager> const & resourceManager,
                     EUsage usage,
                     uint8_t pipelineID)
    : m_resourceManager(resourceManager),
      m_usage(usage),
      m_pipelineID(pipelineID)
  {
    createPacker();
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

  void SkinPage::clearUploadQueue()
  {
    m_uploadQueue.clear();
  }

  void SkinPage::clear()
  {
    clearHandles();
    clearUploadQueue();
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

    m_colorMap[c] = h;

    m2::RectU texRect = m_packer.find(h).second;
    shared_ptr<ResourceStyle> colorStyle(new ColorStyle(texRect, m_pipelineID, c));

    m_styles[h] = colorStyle;
    m_uploadQueue.push_back(colorStyle);

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

  uint32_t SkinPage::findGlyph(GlyphKey const & g) const
  {
    TGlyphMap::const_iterator it = m_glyphMap.find(g);
    if (it == m_glyphMap.end())
      return m_packer.invalidHandle();
    else
      return it->second;
  }

  uint32_t SkinPage::mapGlyph(yg::GlyphKey const & g, yg::GlyphCache * glyphCache)
  {
    uint32_t foundHandle = findGlyph(g);
    if (foundHandle != m_packer.invalidHandle())
      return foundHandle;

    shared_ptr<GlyphInfo> gi = glyphCache->getGlyphInfo(g);

    m2::Packer::handle_t handle = m_packer.pack(gi->m_metrics.m_width + 4,
                                                gi->m_metrics.m_height + 4);

    m2::RectU texRect = m_packer.find(handle).second;
    m_glyphMap[g] = handle;

    boost::shared_ptr<ResourceStyle> glyphStyle(
        new GlyphStyle(texRect,
                      m_pipelineID,
                      gi));

    m_styles[handle] = glyphStyle;
    m_uploadQueue.push_back(glyphStyle);

    return m_glyphMap[g];
  }

  bool SkinPage::hasRoom(GlyphKey const & gk, GlyphCache * glyphCache) const
  {
    shared_ptr<GlyphInfo> gi = glyphCache->getGlyphInfo(gk);
    return m_packer.hasRoom(gi->m_metrics.m_width + 4, gi->m_metrics.m_height + 4);
  }

  bool SkinPage::hasRoom(m2::PointU const * sizes, size_t cnt) const
  {
    return m_packer.hasRoom(sizes, cnt);
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

    m2::PointU sz = circleInfo.patternSize();
    m2::Packer::handle_t handle = m_packer.pack(sz.x, sz.y);

    m_circleInfoMap[circleInfo] = handle;

    m2::RectU texRect = m_packer.find(handle).second;

    shared_ptr<ResourceStyle> circleStyle(new CircleStyle(texRect, m_pipelineID, circleInfo));

    m_styles[handle] = circleStyle;
    m_uploadQueue.push_back(circleStyle);

    return m_circleInfoMap[circleInfo];
  }

  bool SkinPage::hasRoom(CircleInfo const & circleInfo) const
  {
    m2::PointU sz = circleInfo.patternSize();
    return m_packer.hasRoom(sz.x, sz.y);
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

    m_penInfoMap[penInfo] = handle;

    m2::RectU texRect = m_packer.find(handle).second;

    boost::shared_ptr<ResourceStyle> lineStyle(
          new LineStyle(false,
                        texRect,
                        m_pipelineID,
                        penInfo));

    m_styles[handle] = lineStyle;
    m_uploadQueue.push_back(lineStyle);

    return m_penInfoMap[penInfo];
  }

  bool SkinPage::hasRoom(const PenInfo &penInfo) const
  {
    m2::PointU p = penInfo.patternSize();
    return m_packer.hasRoom(p.x, p.y);
  }

  SkinPage::EUsage SkinPage::usage() const
  {
    return m_usage;
  }

  bool SkinPage::hasData()
  {
    return !m_uploadQueue.empty();
  }

  void SkinPage::checkTexture() const
  {
    if ((m_usage != EStaticUsage) && (m_texture == 0))
      reserveTexture();
  }

  void SkinPage::setPipelineID(uint8_t pipelineID)
  {
    m_pipelineID = pipelineID;
    for (TStyles::iterator it = m_styles.begin(); it != m_styles.end(); ++it)
      it->second->m_pipelineID = pipelineID;
  }

  void SkinPage::uploadData()
  {
    if (hasData())
    {
      checkTexture();
      static_cast<gl::ManagedTexture*>(m_texture.get())->lock();

      TDynamicTexture * dynTexture = static_cast<TDynamicTexture*>(m_texture.get());

      for (size_t i = 0; i < m_uploadQueue.size(); ++i)
      {
        shared_ptr<ResourceStyle> const & style = m_uploadQueue[i];

        TDynamicTexture::view_t v = dynTexture->view(style->m_texRect.SizeX(),
                                                     style->m_texRect.SizeY());

        style->render(&v(0, 0));

        dynTexture->upload(&v(0, 0), style->m_texRect);
      }

      m_uploadQueue.clear();

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

  void SkinPage::setTexture(shared_ptr<gl::BaseTexture> const & texture)
  {
    m_texture = texture;
    m_packer = m2::Packer(texture->width(),
                          texture->height(),
                          0x00FFFFFF - 1);
  }

  void SkinPage::freeTexture()
  {
    if (m_texture)
    {
      switch (m_usage)
      {
      case EDynamicUsage:
        m_resourceManager->dynamicTextures()->Free(m_texture);
        break;
      case EFontsUsage:
        m_resourceManager->fontTextures()->Free(m_texture);
        break;
      default:
        LOG(LINFO, ("freeTexture call for with invalid usage param"));
      }

      m_texture.reset();
    }
  }

  void SkinPage::reserveTexture() const
  {
    switch (m_usage)
    {
    case EDynamicUsage:
      m_texture = m_resourceManager->dynamicTextures()->Reserve();
      break;
    case EFontsUsage:
      m_texture = m_resourceManager->fontTextures()->Reserve();
      break;
    default:
      LOG(LINFO, ("freeTexture call for with invalid usage param"));
    }
  }

  void SkinPage::createPacker()
  {
    switch (m_usage)
    {
    case EDynamicUsage:
      m_packer = m2::Packer(m_resourceManager->dynamicTextureWidth(),
                            m_resourceManager->dynamicTextureHeight(),
                            0x00FFFFFF - 1);
      break;
    case EFontsUsage:
      m_packer = m2::Packer(m_resourceManager->fontTextureWidth(),
                            m_resourceManager->fontTextureHeight(),
                            0x00FFFFFF - 1);
      break;
    default:
      LOG(LINFO, ("createPacker call for invalid usage param"));
    }
  }

  shared_ptr<ResourceManager> const & SkinPage::resourceManager() const
  {
    return m_resourceManager;
  }
}

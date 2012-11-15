#include "skin_page.hpp"

#include "opengl/texture.hpp"
#include "opengl/data_traits.hpp"

#include "resource_style.hpp"
#include "resource_manager.hpp"

#include "../base/logging.hpp"

#include "../std/bind.hpp"
#include "../std/numeric.hpp"

namespace graphics
{
  typedef gl::Texture<DATA_TRAITS, true> TDynamicTexture;

  SkinPage::SkinPage()
    : m_type(EStatic),
      m_pipelineID(0)
  {}

  SkinPage::SkinPage(shared_ptr<ResourceManager> const & resourceManager,
                     char const * name,
                     uint8_t pipelineID)
                   : m_texture(resourceManager->getTexture(name)),
                     m_packer(m_texture->width(), m_texture->height(), 0x00FFFFFF - 1),
                     m_type(EStatic),
                     m_pipelineID(pipelineID)
  {
  }

  SkinPage::SkinPage(shared_ptr<ResourceManager> const & resourceManager,
                     EType type,
                     uint8_t pipelineID)
    : m_resourceManager(resourceManager),
      m_type(type),
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
    clearImageInfoHandles();

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

  void SkinPage::clearImageInfoHandles()
  {
    for (TImageInfoMap::const_iterator it = m_imageInfoMap.begin();
         it != m_imageInfoMap.end();
         ++it)
      m_styles.erase(it->second);

    m_imageInfoMap.clear();
  }

  uint32_t SkinPage::findImageInfo(ImageInfo const & ii) const
  {
    TImageInfoMap::const_iterator it = m_imageInfoMap.find(ii);
    if (it == m_imageInfoMap.end())
      return m_packer.invalidHandle();
    else
      return it->second;
  }

  uint32_t SkinPage::mapImageInfo(ImageInfo const & ii)
  {
    uint32_t foundHandle = findImageInfo(ii);
    if (foundHandle != m_packer.invalidHandle())
      return foundHandle;

    m2::Packer::handle_t h = m_packer.pack(ii.width() + 4, ii.height() + 4);

    m_imageInfoMap[ii] = h;

    m2::RectU texRect = m_packer.find(h).second;
    shared_ptr<ResourceStyle> imageStyle(new ImageStyle(texRect, m_pipelineID, ii));

    m_styles[h] = imageStyle;
    m_uploadQueue.push_back(imageStyle);

    return h;
  }

  bool SkinPage::hasRoom(ImageInfo const & ii) const
  {
    return m_packer.hasRoom(ii.width() + 4, ii.height() + 4);
  }

  uint32_t SkinPage::findColor(graphics::Color const & c) const
  {
    TColorMap::const_iterator it = m_colorMap.find(c);
    if (it == m_colorMap.end())
      return m_packer.invalidHandle();
    else
      return it->second;
  }

  uint32_t SkinPage::mapColor(graphics::Color const & c)
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

  uint32_t SkinPage::mapGlyph(graphics::GlyphKey const & g, graphics::GlyphCache * glyphCache)
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

  void SkinPage::setType(SkinPage::EType type)
  {
    m_type = type;
    createPacker();
    if (m_type != EStatic)
      m_packer.addOverflowFn(bind(&SkinPage::clearHandles, this), 0);
  }

  SkinPage::EType SkinPage::type() const
  {
    return m_type;
  }

  bool SkinPage::hasData()
  {
    return !m_uploadQueue.empty();
  }

  SkinPage::TUploadQueue const & SkinPage::uploadQueue() const
  {
    return m_uploadQueue;
  }

  void SkinPage::checkTexture() const
  {
    if ((m_type != EStatic) && (m_texture == 0))
      reserveTexture();
  }

  void SkinPage::setPipelineID(uint8_t pipelineID)
  {
    m_pipelineID = pipelineID;
    for (TStyles::iterator it = m_styles.begin(); it != m_styles.end(); ++it)
      it->second->m_pipelineID = pipelineID;
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

  bool SkinPage::hasTexture() const
  {
    return m_texture != 0;
  }

  void SkinPage::setTexture(shared_ptr<gl::BaseTexture> const & texture)
  {
    m_texture = texture;
    m_packer = m2::Packer(texture->width(),
                          texture->height(),
                          0x00FFFFFF - 1);
  }

  void SkinPage::resetTexture()
  {
    m_texture.reset();
  }

  void SkinPage::reserveTexture() const
  {
    switch (m_type)
    {
    case EPrimary:
      m_texture = m_resourceManager->primaryTextures()->Reserve();
      break;
    case EFonts:
      m_texture = m_resourceManager->fontTextures()->Reserve();
      break;
    case ELightWeight:
      m_texture = m_resourceManager->guiThreadTextures()->Reserve();
      break;
    default:
      LOG(LINFO, ("reserveTexture call for with invalid type param"));
    }
  }

  void SkinPage::createPacker()
  {
    switch (m_type)
    {
    case EPrimary:
      m_packer = m2::Packer(m_resourceManager->params().m_primaryTexturesParams.m_texWidth,
                            m_resourceManager->params().m_primaryTexturesParams.m_texHeight,
                            0x00FFFFFF - 1);
      break;
    case EFonts:
      m_packer = m2::Packer(m_resourceManager->params().m_fontTexturesParams.m_texWidth,
                            m_resourceManager->params().m_fontTexturesParams.m_texHeight,
                            0x00FFFFFF - 1);
      break;
    case ELightWeight:
      m_packer = m2::Packer(m_resourceManager->params().m_guiThreadTexturesParams.m_texWidth,
                            m_resourceManager->params().m_guiThreadTexturesParams.m_texHeight,
                            0x00FFFFFF - 1);
      break;
    default:
      LOG(LINFO, ("createPacker call for invalid type param"));
    }
  }

  shared_ptr<ResourceManager> const & SkinPage::resourceManager() const
  {
    return m_resourceManager;
  }
}

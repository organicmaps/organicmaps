#pragma once

#include "../std/shared_ptr.hpp"

#include "../geometry/packer.hpp"
#include "../geometry/rect2d.hpp"

#include "pen_info.hpp"
#include "circle_info.hpp"
#include "color.hpp"
#include "glyph_cache.hpp"

namespace yg
{
  namespace gl
  {
    class BaseTexture;
  }

  struct GlyphStyle;
  struct ResourceStyle;
  class ResourceManager;
  struct GlyphInfo;

  struct FontInfo
  {
    int8_t m_fontSize;
    typedef map<int32_t, pair<shared_ptr<GlyphStyle>, shared_ptr<GlyphStyle> > > TChars;
    TChars m_chars;

    mutable pair<ResourceStyle *, ResourceStyle *> m_invalidChar;
    FontInfo() : m_invalidChar(static_cast<ResourceStyle *>(0), static_cast<ResourceStyle *>(0)) {}

    ResourceStyle * fromID(uint32_t id, bool isMask = false) const;
  };

  class SkinPage
  {
  public:

    enum EType
    {
      EStatic,
      EPrimary,
      EFonts,
      ELightWeight
    };

    typedef m2::Packer::overflowFn overflowFn;

    typedef vector<shared_ptr<ResourceStyle> > TUploadQueue;

  private:

    typedef map<uint32_t, shared_ptr<ResourceStyle> > TStyles;
    TStyles m_styles;

    typedef map<string, uint32_t> TPointNameMap;
    TPointNameMap m_pointNameMap;

    typedef map<PenInfo, uint32_t> TPenInfoMap;
    TPenInfoMap m_penInfoMap;

    typedef map<CircleInfo, uint32_t> TCircleInfoMap;
    TCircleInfoMap m_circleInfoMap;

    typedef map<Color, uint32_t> TColorMap;
    TColorMap m_colorMap;

    typedef map<GlyphKey, uint32_t> TGlyphMap;
    TGlyphMap m_glyphMap;

    /// made mutable to implement lazy reservation of texture
    /// @{
    mutable shared_ptr<gl::BaseTexture> m_texture;
    mutable shared_ptr<ResourceManager> m_resourceManager;
    /// @}

    m2::Packer m_packer;

    TUploadQueue m_uploadQueue;

    typedef vector<FontInfo> TFonts;
    TFonts m_fonts;

    EType m_type;
    uint32_t m_pipelineID;

    bool m_fillAlpha;

    /// number of pending rendering commands,
    /// that are using this skin_page
    uint32_t m_activeCommands;

    friend class SkinLoader;

  public:

    void clearColorHandles();
    void clearPenInfoHandles();
    void clearFontHandles();
    void clearCircleInfoHandles();

    void clearHandles();

    void clear();

    bool hasData();
    TUploadQueue const & uploadQueue() const;
    void clearUploadQueue();

    void uploadData();

    void checkTexture() const;
    void setPipelineID(uint8_t pipelineID);

    /// creation of detached page
    SkinPage();

    /// creation of a static page
    SkinPage(shared_ptr<ResourceManager> const & resourceManager,
             char const * name,
             uint8_t pipelineID);

    /// creation of a dynamic page
    SkinPage(shared_ptr<ResourceManager> const & resourceManager,
             EType type,
             uint8_t pipelineID);

    void reserveTexture() const;
    void freeTexture();
    void resetTexture();
    void createPacker();

    uint32_t findColor(Color const & c) const;
    uint32_t mapColor(Color const & c);
    bool     hasRoom(Color const & c) const;

    uint32_t findPenInfo(PenInfo const & penInfo) const;
    uint32_t mapPenInfo(PenInfo const & penInfo);
    bool     hasRoom(PenInfo const & penInfo) const;

    uint32_t findCircleInfo(CircleInfo const & circleInfo) const;
    uint32_t mapCircleInfo(CircleInfo const & circleInfo);
    bool hasRoom(CircleInfo const & circleInfo) const;

    uint32_t findGlyph(GlyphKey const & g) const;
    uint32_t mapGlyph(GlyphKey const & g, GlyphCache * glyphCache);
    bool hasRoom(GlyphKey const & g, GlyphCache * glyphCache) const;
    bool hasRoom(m2::PointU const * sizes, size_t cnt) const;

    uint32_t findSymbol(char const * symbolName) const;

    ResourceStyle * fromID(uint32_t idx) const;

    void setType(EType type);
    EType type() const;
    shared_ptr<ResourceManager> const & resourceManager() const;

    void addOverflowFn(overflowFn fn, int priority);

    bool hasTexture() const;
    shared_ptr<gl::BaseTexture> const & texture() const;
    void setTexture(shared_ptr<gl::BaseTexture> const & t);
  };
}

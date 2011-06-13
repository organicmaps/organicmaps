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

  struct CharStyle;
  struct ResourceStyle;
  class ResourceManager;
  struct GlyphInfo;

  struct GlyphUploadCmd
  {
    shared_ptr<GlyphInfo> m_glyphInfo;
    m2::RectU m_rect;
    GlyphUploadCmd(shared_ptr<GlyphInfo> const & glyphInfo, m2::RectU const & rect);
    GlyphUploadCmd();
  };

  struct ColorUploadCmd
  {
    yg::Color m_color;
    m2::RectU m_rect;
    ColorUploadCmd(yg::Color const & color, m2::RectU const & rect);
    ColorUploadCmd();
  };

  struct PenUploadCmd
  {
    yg::PenInfo m_penInfo;
    m2::RectU m_rect;
    PenUploadCmd(yg::PenInfo const & penInfo, m2::RectU const & rect);
    PenUploadCmd();
  };

  struct CircleUploadCmd
  {
    yg::CircleInfo m_circleInfo;
    m2::RectU m_rect;
    CircleUploadCmd(yg::CircleInfo const & circleInfo, m2::RectU const & rect);
    CircleUploadCmd();
  };

  struct FontInfo
  {
    int8_t m_fontSize;
    typedef map<int32_t, pair<shared_ptr<CharStyle>, shared_ptr<CharStyle> > > TChars;
    TChars m_chars;

    mutable pair<ResourceStyle *, ResourceStyle *> m_invalidChar;
    FontInfo() : m_invalidChar(static_cast<ResourceStyle *>(0), static_cast<ResourceStyle *>(0)) {}

    ResourceStyle * fromID(uint32_t id, bool isMask = false) const;
  };

  class SkinPage
  {
  public:

    enum EUsage
    {
      EStaticUsage,
      EDynamicUsage,
      EFontsUsage
    };

    typedef m2::Packer::overflowFn overflowFn;

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

    m2::Packer m_packer;
    /// made mutable to implement lazy reservation of texture
    /// @{
    mutable shared_ptr<gl::BaseTexture> m_texture;
    mutable shared_ptr<ResourceManager> m_resourceManager;
    /// @}

    vector<ColorUploadCmd> m_colorUploadCommands;
    vector<PenUploadCmd> m_penUploadCommands;
    vector<GlyphUploadCmd> m_glyphUploadCommands;
    vector<CircleUploadCmd> m_circleUploadCommands;

    void uploadPenInfo();
    void uploadColors();
    void uploadGlyphs();
    void uploadCircleInfo();

    typedef vector<FontInfo> TFonts;
    TFonts m_fonts;

    EUsage m_usage;
    uint32_t m_pageID;

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

    bool hasData();
    void uploadData();

    void checkTexture() const;

    SkinPage();

    /// creation of a static page
    SkinPage(shared_ptr<ResourceManager> const & resourceManager,
             char const * name,
             uint8_t pageID,
             bool fillAlpha);

    /// creation of a dynamic page
    SkinPage(shared_ptr<ResourceManager> const & resourceManager,
             EUsage usage,
             uint8_t pageID,
             bool fillAlpha);

    void reserveTexture() const;
    void freeTexture();
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

    uint32_t findSymbol(char const * symbolName) const;

    ResourceStyle * fromID(uint32_t idx) const;

    EUsage usage() const;

    void addOverflowFn(overflowFn fn, int priority);

    shared_ptr<gl::BaseTexture> const & texture() const;
  };
}

#pragma once

#include "../std/shared_ptr.hpp"
#include "../std/unordered_map.hpp"
#include "../std/map.hpp"
#include "../std/vector.hpp"

#include "../geometry/packer.hpp"
#include "../geometry/rect2d.hpp"

#include "pen_info.hpp"
#include "color.hpp"

namespace yg
{
  namespace gl
  {
    class BaseTexture;
  }

  struct CharStyle;
  struct ResourceStyle;
  class ResourceManager;

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

  struct FontInfo
  {
    int8_t m_fontSize;
    typedef map<int32_t, shared_ptr<CharStyle> > TChars;
    TChars m_chars;

    mutable ResourceStyle * m_invalidChar;
    FontInfo() : m_invalidChar(0){}

    ResourceStyle * fromID(uint32_t id) const;
  };

  class SkinPage
  {
  public:

    typedef m2::Packer::overflowFn overflowFn;

  private:

//    typedef unordered_map<uint32_t, shared_ptr<ResourceStyle> > TStyles;
    typedef map<uint32_t, shared_ptr<ResourceStyle> > TStyles;
    TStyles m_styles;

//    typedef unordered_map<string, uint32_t> TPointNameMap;
    typedef map<string, uint32_t> TPointNameMap;
    TPointNameMap m_pointNameMap;

    typedef map<PenInfo, uint32_t> TPenInfoMap;
    TPenInfoMap m_penInfoMap;

    typedef map<Color, uint32_t> TColorMap;
    TColorMap m_colorMap;

    m2::Packer m_packer;
    shared_ptr<gl::BaseTexture> m_texture;
    shared_ptr<ResourceManager> m_resourceManager;

    vector<ColorUploadCmd> m_colorUploadCommands;
    vector<PenUploadCmd> m_penUploadCommands;

    void uploadPenInfo();
    void uploadColors();

    vector<FontInfo> m_fonts;

    bool m_isDynamic;
    uint32_t m_pageID;

    /// number of pending rendering commands,
    /// that are using this skin_page
    uint32_t m_activeCommands;

    friend class SkinLoader;

  public:

    void clearHandles();

    void uploadData();

    SkinPage();

    /// creation of a static page
    SkinPage(shared_ptr<ResourceManager> const & resourceManager,
             char const * name,
             uint8_t pageID);

    /// creation of a dynamic page
    SkinPage(shared_ptr<ResourceManager> const & resourceManager,
             uint8_t pageID);

    void reserveTexture();
    void freeTexture();

    uint32_t find(Color const & c) const;
    uint32_t Map(Color const & c);
    bool     hasRoom(Color const & c) const;

    uint32_t find(PenInfo const & penInfo) const;
    uint32_t Map(PenInfo const & penInfo);
    bool     hasRoom(PenInfo const & penInfo) const;

    uint32_t find(char const * symbolName) const;

    ResourceStyle * fromID(uint32_t idx) const;

    vector<FontInfo> const & fonts() const;

    bool isDynamic() const;

    void addOverflowFn(overflowFn fn, int priority);

    shared_ptr<gl::BaseTexture> const & texture() const;
  };
}

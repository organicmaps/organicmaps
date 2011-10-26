/// @author Siarhei Rachytski
#pragma once

//#include "resource_style.hpp"
//#include "skin.hpp"
//#include "../geometry/rect2d.hpp"
//#include "../base/base.hpp"
//#include "../std/string.hpp"
//#include "../std/list.hpp"
//#include "../std/map.hpp"
//#include "../std/vector.hpp"
#include "../std/shared_ptr.hpp"
//#include "../std/utility.hpp"

/// @example
/// <?xml version="1.0" ?>
/// <skin>
///	<texture id="0" height="512" width="512" file="basic.png">
///		<fontStyle charID="-1" xAdvance="17" xOffset="-1" yOffset="-7">
///			<resourceStyle height="29" width="18" x="152" y="93"/>
///		</fontStyle>
///   ...
///		<fontStyle charID="35" xAdvance="23" xOffset="0" yOffset="-2">
///			<resourceStyle height="25" width="23" x="537" y="171"/>
///		</fontStyle>
/// </texture>
/// <texture id="1" height="512" width="512" file="dejavu-sans-12pt.png">
/// ...
/// </texture>
/// </skin>

namespace yg
{
  namespace gl
  {
    class BaseTexture;
  }

  class ResourceManager;
  class SkinPage;
  struct ResourceStyle;
  struct GlyphStyle;

  class SkinLoader
  {
  private:

    enum EMode
    {
      ERoot,
      EPage,
      ESkin,
      EFontStyle,
      EPointStyle,
      ELineStyle,
      EResourceStyle
    };

    list<EMode> m_mode;

/// resourceStyle-specific parameters
    int32_t m_id;
    uint32_t m_texX;
    uint32_t m_texY;
    uint32_t m_texWidth;
    uint32_t m_texHeight;
    m2::RectU m_texRect;

/// glyphInfo and glyphMaskInfo specific parameters
    int8_t m_xOffset;
    int8_t m_yOffset;
    int8_t m_xAdvance;

    shared_ptr<GlyphStyle> m_glyphInfo;
    shared_ptr<GlyphStyle> m_glyphMaskInfo;

/// pointStyle-specific parameters
    string m_styleID;

/// skin-specific parameters

    vector<shared_ptr<SkinPage> > m_pages;

/// skin-page specific parameters
    string m_fileName;

    typedef list<pair<int32_t, shared_ptr<ResourceStyle> > > TStylesList;

    TStylesList m_stylesList;

    shared_ptr<ResourceManager> m_resourceManager;
    Skin * m_skin;
    size_t m_dynamicPagesCount;
    size_t m_textPagesCount;

  public:

    SkinLoader(shared_ptr<ResourceManager> const & resourceManager,
               size_t dynamicPagesCount,
               size_t textPagesCount);

    bool Push(string const & element);
    void Pop(string const & element);
    void AddAttr(string const & attribute, string const & value);
    void CharData(string const &) {}

    void popPointStyle();
    void popSkin();
    void pushPage();
    void popPage();

    void pushResourceStyle();
    void popResourceStyle();

    Skin * skin();
  };
}

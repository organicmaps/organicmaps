#include "skin.hpp"
#include "skin_loader.hpp"
#include "resource_manager.hpp"

#include "../base/string_utils.hpp"
#include "../base/start_mem_debug.hpp"


namespace yg
{
  SkinLoader::SkinLoader(shared_ptr<ResourceManager> const & resourceManager,
                         size_t dynamicPagesCount,
                         size_t textPagesCount)
    : m_id(-1),
    m_texRect(0, 0, 0, 0),
    m_xOffset(0),
    m_yOffset(0),
    m_xAdvance(0),
    m_fileName(""),
    m_resourceManager(resourceManager),
    m_skin(0),
    m_dynamicPagesCount(dynamicPagesCount),
    m_textPagesCount(textPagesCount)
  {
    m_mode.push_back(ERoot);
  }

  void SkinLoader::pushResourceStyle()
  {
    m_texRect = m2::RectU(0, 0, 0, 0);
  }

  void SkinLoader::popResourceStyle()
  {
    m_texRect = m2::RectU(m_texX, m_texY, m_texX + m_texWidth, m_texY + m_texHeight);
  }

  void SkinLoader::popCharStyle()
  {
    m_chars[m_id] = make_pair(m_glyphInfo, m_glyphMaskInfo);
  }

  void SkinLoader::popGlyphInfo()
  {
    m_glyphInfo = shared_ptr<CharStyle>(new CharStyle(m_texRect, m_pages.size(), m_xOffset, m_yOffset, m_xAdvance));
  }

  void SkinLoader::popGlyphMaskInfo()
  {
    m_glyphMaskInfo = shared_ptr<CharStyle>(new CharStyle(m_texRect, m_pages.size(), m_xOffset, m_yOffset, m_xAdvance));
  }

  void SkinLoader::pushFontInfo()
  {
    m_chars.clear();
    m_fontSize = 0;
  }

  void SkinLoader::popFontInfo()
  {
    m_fonts[m_fontSize] = m_chars;
  }

  void SkinLoader::popPointStyle()
  {
    uint32_t id = m_id;
    pair<int32_t, shared_ptr<ResourceStyle> > style(
        id, shared_ptr<PointStyle>(new PointStyle(m_texRect, m_pages.size(), m_styleID)));
    m_stylesList.push_back(style);
  }

  void SkinLoader::pushPage()
  {
    m_stylesList.clear();
  }

  void SkinLoader::popPage()
  {
    m_pages.push_back(make_shared_ptr(new SkinPage(m_resourceManager, m_fileName.c_str(), m_pages.size(), m_resourceManager->fillSkinAlpha())));

    TStylesList::iterator prevIt = m_stylesList.end();

    for (TStylesList::iterator it = m_stylesList.begin(); it != m_stylesList.end(); ++it)
    {
      m_pages.back()->m_styles[it->first] = it->second;

      if (it->second->m_cat == ResourceStyle::EPointStyle)
        m_pages.back()->m_pointNameMap[static_cast<PointStyle*>(it->second.get())->m_styleName] = it->first;

      if (prevIt != m_stylesList.end())
        m_stylesList.erase(prevIt);
      prevIt = it;
    }

    for (TFonts::const_iterator it = m_fonts.begin(); it != m_fonts.end(); ++it)
    {
      FontInfo fontInfo;
      fontInfo.m_fontSize = it->first;
      fontInfo.m_chars = it->second;
      m_pages.back()->m_fonts.push_back(fontInfo);
    }

    m_fonts.clear();
  }

  void SkinLoader::popSkin()
  {
    m_skin = new Skin(m_resourceManager, m_pages, m_dynamicPagesCount, m_textPagesCount);
  }

#define PUSH_MODE(mode, name) \
  if (element == name) \
    m_mode.push_back(mode);\

#define PUSH_MODE_EX(mode, name, fn)\
  if (element == name)\
  {\
    m_mode.push_back(mode);\
    fn();\
  }

#define POP_MODE(mode, name) \
  if (element == name) \
    m_mode.pop_back();


#define POP_MODE_EX(mode, name, f) \
  if (element == name)\
  {\
      f();\
      m_mode.pop_back();\
  }

  bool SkinLoader::Push(string const & element)
  {
    PUSH_MODE(ESkin, "skin");
    PUSH_MODE_EX(EPage, "page", pushPage);
    PUSH_MODE(ECharStyle, "charStyle");
    PUSH_MODE(EGlyphInfo, "glyphInfo");
    PUSH_MODE(EGlyphMaskInfo, "glyphMaskInfo");
    PUSH_MODE_EX(EFontInfo, "fontInfo", pushFontInfo);
    PUSH_MODE(EPointStyle, "symbolStyle");
    PUSH_MODE(ELineStyle, "lineStyle");
    PUSH_MODE_EX(EResourceStyle, "resourceStyle", pushResourceStyle);
    return true;
  }

  void SkinLoader::Pop(string const & element)
  {
    POP_MODE_EX(ESkin, "skin", popSkin);
    POP_MODE_EX(EPage, "page", popPage);
    POP_MODE_EX(ECharStyle, "charStyle", popCharStyle);
    POP_MODE_EX(EGlyphInfo, "glyphInfo", popGlyphInfo);
    POP_MODE_EX(EGlyphMaskInfo, "glyphMaskInfo", popGlyphMaskInfo);
    POP_MODE_EX(EFontInfo, "fontInfo", popFontInfo);
    POP_MODE_EX(EPointStyle, "symbolStyle", popPointStyle);
    POP_MODE(ELineStyle, "lineStyle");
    POP_MODE_EX(EResourceStyle, "resourceStyle", popResourceStyle);
  }

  int StrToInt(string const & s)
  {
    int i;
    VERIFY ( string_utils::to_int(s, i), ("Bad int int StrToInt function") );
    return i;
  }

  void SkinLoader::AddAttr(string const & attr, string const & value)
  {
    switch (m_mode.back())
    {
    case ESkin:
      break;
    case EPage:
      if (attr == "file")
        m_fileName = value;
      break;
    case ECharStyle:
      if (attr == "id")
        m_id = StrToInt(value);
    case EGlyphInfo: case EGlyphMaskInfo:
      if (attr == "xOffset")
        m_xOffset = StrToInt(value);
      else if (attr == "yOffset")
        m_yOffset = StrToInt(value);
      else if (attr == "xAdvance")
        m_xAdvance = StrToInt(value);
      break;
    case EFontInfo:
      if (attr == "size")
        m_fontSize = StrToInt(value);
      break;
    case EPointStyle:
      if (attr == "id")
        m_id = StrToInt(value);
      else if (attr == "name")
        m_styleID = value;
      break;
    case EResourceStyle:
      if (attr == "x")
        m_texX = StrToInt(value);
      else if (attr == "y")
        m_texY = StrToInt(value);
      else if (attr == "height")
        m_texHeight = StrToInt(value);
      else if (attr == "width")
        m_texWidth = StrToInt(value);
      break;
    default:
      break;
    }
  }

  Skin * SkinLoader::skin()
  {
    return m_skin;
  }
}

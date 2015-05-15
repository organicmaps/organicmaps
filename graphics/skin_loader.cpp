#include "graphics/skin_loader.hpp"

#include "base/string_utils.hpp"

namespace graphics
{
  SkinLoader::SkinLoader(TIconCallback const & callback)
  : m_id(-1)
  , m_fileName("")
  , m_callback(callback)
  {
    m_mode.push_back(ERoot);
  }

  void SkinLoader::popIcon()
  {
    m_callback(m2::RectU(m_texX, m_texY, m_texX + m_texWidth, m_texY + m_texHeight),
               m_resID, m_id, m_fileName);
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
    PUSH_MODE(EPage, "page");
    PUSH_MODE(EIcon, "symbolStyle");
    PUSH_MODE(EResource, "resourceStyle");
    return true;
  }

  void SkinLoader::Pop(string const & element)
  {
    POP_MODE(ESkin, "skin");
    POP_MODE(EPage, "page");
    POP_MODE_EX(EIcon, "symbolStyle", popIcon);
    POP_MODE(EResource, "resourceStyle");
  }

  int StrToInt(string const & s)
  {
    int i;
    VERIFY ( strings::to_int(s, i), ("Bad int int StrToInt function") );
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
    case EIcon:
      if (attr == "id")
        m_id = StrToInt(value);
      else if (attr == "name")
        m_resID = value;
      break;
    case EResource:
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
}

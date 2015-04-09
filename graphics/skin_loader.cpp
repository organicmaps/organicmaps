#include "graphics/skin_loader.hpp"
#include "graphics/resource_manager.hpp"
#include "graphics/resource_cache.hpp"
#include "graphics/resource.hpp"
#include "graphics/icon.hpp"

#include "base/string_utils.hpp"

namespace graphics
{
  SkinLoader::SkinLoader(shared_ptr<ResourceManager> const & resourceManager,
                         vector<shared_ptr<ResourceCache> > & caches)
  : m_id(-1),
    m_texRect(0, 0, 0, 0),
    m_fileName(""),
    m_resourceManager(resourceManager),
    m_caches(caches)
  {
    m_mode.push_back(ERoot);
  }

  void SkinLoader::pushResource()
  {
    m_texRect = m2::RectU(0, 0, 0, 0);
  }

  void SkinLoader::popResource()
  {
    m_texRect = m2::RectU(m_texX,
                          m_texY,
                          m_texX + m_texWidth,
                          m_texY + m_texHeight);
  }

  void SkinLoader::popIcon()
  {
    shared_ptr<Resource> res(
          new Icon(m_texRect, m_caches.size(), Icon::Info(m_resID)));

    pair<int32_t, shared_ptr<Resource> > p(m_id, res);
    m_resourceList.push_back(p);
  }

  void SkinLoader::pushPage()
  {
    m_resourceList.clear();
  }

  void SkinLoader::popPage()
  {
    m_caches.push_back(make_shared<ResourceCache>(m_resourceManager, m_fileName, m_caches.size()));

    TResourceList::iterator prevIt = m_resourceList.end();

    for (TResourceList::iterator it = m_resourceList.begin();
         it != m_resourceList.end();
         ++it)
    {
      m_caches.back()->m_resources[it->first] = it->second;

      if (it->second->m_cat == Resource::EIcon)
      {
        Icon * icon = static_cast<Icon*>(it->second.get());
        m_caches.back()->m_infos[&icon->m_info] = it->first;
      }

      if (prevIt != m_resourceList.end())
        m_resourceList.erase(prevIt);
      prevIt = it;
    }
  }

  void SkinLoader::popSkin()
  {
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
    PUSH_MODE(EIcon, "symbolStyle");
    PUSH_MODE_EX(EResource, "resourceStyle", pushResource);
    return true;
  }

  void SkinLoader::Pop(string const & element)
  {
    POP_MODE_EX(ESkin, "skin", popSkin);
    POP_MODE_EX(EPage, "page", popPage);
    POP_MODE_EX(EIcon, "symbolStyle", popIcon);
    POP_MODE_EX(EResource, "resourceStyle", popResource);
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

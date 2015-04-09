/// @author Siarhei Rachytski
#pragma once

#include "graphics/defines.hpp"

#include "geometry/rect2d.hpp"
#include "std/list.hpp"
#include "std/shared_ptr.hpp"

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

namespace graphics
{
  namespace gl
  {
    class BaseTexture;
  }

  class ResourceManager;
  class ResourceCache;
  struct Resource;

  class SkinLoader
  {
  private:

    enum EMode
    {
      ERoot,
      EPage,
      ESkin,
      EIcon,
      EResource
    };

    list<EMode> m_mode;

    /// resourceStyle-specific parameters
    int32_t m_id;
    uint32_t m_texX;
    uint32_t m_texY;
    uint32_t m_texWidth;
    uint32_t m_texHeight;
    m2::RectU m_texRect;

    /// pointStyle-specific parameters
    string m_resID;

    /// skin-page specific parameters
    string m_fileName;

    shared_ptr<ResourceManager> m_resourceManager;

    /// skin-specific parameters

    vector<shared_ptr<ResourceCache> > & m_caches;


    typedef list<pair<int32_t, shared_ptr<Resource> > > TResourceList;

    TResourceList m_resourceList;

  public:

    SkinLoader(shared_ptr<ResourceManager> const & resourceManager,
               vector<shared_ptr<ResourceCache> > & caches);

    bool Push(string const & element);
    void Pop(string const & element);
    void AddAttr(string const & attribute, string const & value);
    void CharData(string const &) {}

    void popIcon();
    void popSkin();
    void pushPage();
    void popPage();

    void pushResource();
    void popResource();

    vector<shared_ptr<ResourceCache> > const & caches();
  };
}

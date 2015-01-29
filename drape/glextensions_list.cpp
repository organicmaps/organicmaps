#include "drape/glextensions_list.hpp"
#include "drape/glfunctions.hpp"

#include "base/assert.hpp"

#include "std/string.hpp"

namespace dp
{

#ifdef DEBUG
  #include "../std/map.hpp"

  class GLExtensionsList::Impl
  {
  public:
    void CheckExtension(GLExtensionsList::ExtensionName const & enumName, const string & extName)
    {
#ifdef OMIM_OS_ANDROID
      if (enumName == GLExtensionsList::VertexArrayObject)
        m_supportedMap[enumName] = false;
      else
#endif
        m_supportedMap[enumName] = GLFunctions::glHasExtension(extName);
    }

    bool IsSupported(GLExtensionsList::ExtensionName const & enumName) const
    {
      map<GLExtensionsList::ExtensionName, bool>::const_iterator it = m_supportedMap.find(enumName);
      if (it != m_supportedMap.end())
        return it->second;

      ASSERT(false, ("Not all used extensions is checked"));
      return false;
    }

  private:
    map<GLExtensionsList::ExtensionName, bool> m_supportedMap;
  };
#else
  #include "../std/set.hpp"

  class GLExtensionsList::Impl
  {
  public:
    void CheckExtension(GLExtensionsList::ExtensionName const & enumName, const string & extName)
    {
#ifdef OMIM_OS_ANDROID
      if (enumName == GLExtensionsList::VertexArrayObject)
        return;
#endif
      if (GLFunctions::glHasExtension(extName))
        m_supported.insert(enumName);
    }

    bool IsSupported(GLExtensionsList::ExtensionName const & enumName) const
    {
      if (m_supported.find(enumName) != m_supported.end())
        return true;

      return false;
    }

  private:
    set<GLExtensionsList::ExtensionName> m_supported;
  };
#endif

GLExtensionsList::GLExtensionsList()
  : m_impl(new Impl())
{
#if defined(OMIM_OS_MOBILE)
  m_impl->CheckExtension(VertexArrayObject, "GL_OES_vertex_array_object");
  m_impl->CheckExtension(TextureNPOT, "GL_OES_texture_npot");
  m_impl->CheckExtension(RequiredInternalFormat, "GL_OES_required_internalformat");
  m_impl->CheckExtension(MapBuffer, "GL_OES_mapbuffer");
#else
  m_impl->CheckExtension(VertexArrayObject, "GL_APPLE_vertex_array_object");
  m_impl->CheckExtension(TextureNPOT, "GL_ARB_texture_non_power_of_two");
  m_impl->CheckExtension(RequiredInternalFormat, "GL_OES_required_internalformat");
  m_impl->CheckExtension(MapBuffer, "GL_OES_mapbuffer");
#endif
}

GLExtensionsList::~GLExtensionsList()
{
  delete m_impl;
}

GLExtensionsList & GLExtensionsList::Instance()
{
  static GLExtensionsList extList;
  return extList;
}

bool GLExtensionsList::IsSupported(ExtensionName const & extName) const
{
  return m_impl->IsSupported(extName);
}

} // namespace dp

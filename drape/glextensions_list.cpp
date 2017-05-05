#include "drape/glextensions_list.hpp"
#include "drape/glfunctions.hpp"

#include "base/assert.hpp"

#include "std/target_os.hpp"

namespace dp
{
GLExtensionsList::GLExtensionsList(dp::ApiVersion apiVersion)
{
#if defined(OMIM_OS_MOBILE)
  CheckExtension(TextureNPOT, "GL_OES_texture_npot");
  if (apiVersion == dp::ApiVersion::OpenGLES2)
  {
#ifdef OMIM_OS_ANDROID
    SetExtension(VertexArrayObject, false);
#else
    CheckExtension(VertexArrayObject, "GL_OES_vertex_array_object");
#endif
    CheckExtension(MapBuffer, "GL_OES_mapbuffer");
    CheckExtension(UintIndices, "GL_OES_element_index_uint");
    CheckExtension(MapBufferRange, "GL_EXT_map_buffer_range");
  }
  else
  {
    SetExtension(VertexArrayObject, true);
    SetExtension(MapBuffer, true);
    SetExtension(MapBufferRange, true);
    SetExtension(UintIndices, true);
  }
#elif defined(OMIM_OS_WINDOWS)
  m_impl->CheckExtension(TextureNPOT, "GL_ARB_texture_non_power_of_two");
  SetExtension(MapBuffer, true);
  SetExtension(UintIndices, true);
  if (apiVersion == dp::ApiVersion::OpenGLES2)
  {
    SetExtension(VertexArrayObject, false);
    SetExtension(MapBufferRange, false);
  }
  else
  {
    SetExtension(VertexArrayObject, true);
    SetExtension(MapBufferRange, true);
  }
#else
  CheckExtension(TextureNPOT, "GL_ARB_texture_non_power_of_two");
  SetExtension(MapBuffer, true);
  SetExtension(UintIndices, true);
  if (apiVersion == dp::ApiVersion::OpenGLES2)
  {
    CheckExtension(VertexArrayObject, "GL_APPLE_vertex_array_object");
    SetExtension(MapBufferRange, false);
  }
  else
  {
    SetExtension(VertexArrayObject, true);
    SetExtension(MapBufferRange, true);
  }
#endif
}

// static
GLExtensionsList & GLExtensionsList::Instance()
{
  static GLExtensionsList extList(GLFunctions::CurrentApiVersion);
  return extList;
}

bool GLExtensionsList::IsSupported(ExtensionName extName) const
{
  auto const it = m_supportedMap.find(extName);
  if (it != m_supportedMap.end())
    return it->second;

  ASSERT(false, ("Not all used extensions are checked"));
  return false;
}

void GLExtensionsList::CheckExtension(ExtensionName enumName, std::string const & extName)
{
  m_supportedMap[enumName] = GLFunctions::glHasExtension(extName);
}

void GLExtensionsList::SetExtension(ExtensionName enumName, bool isSupported)
{
  m_supportedMap[enumName] = isSupported;
}
}  // namespace dp

#include "drape/gl_extensions_list.hpp"
#include "drape/gl_functions.hpp"

#include "base/assert.hpp"

#include "std/target_os.hpp"

namespace dp
{
void GLExtensionsList::Init(dp::ApiVersion apiVersion)
{
#if defined(OMIM_OS_MOBILE)
  if (apiVersion == dp::ApiVersion::OpenGLES2)
  {
#ifdef OMIM_OS_ANDROID
    SetExtension(VertexArrayObject, false);
    // On some Android devices glMapBufferRange/glMapBuffer works very slow.
    // We have to substitute these functions to glBufferData/glBufferSubData.
    SetExtension(MapBuffer, false);
    SetExtension(MapBufferRange, false);
#else
    CheckExtension(VertexArrayObject, "GL_OES_vertex_array_object");
    CheckExtension(MapBuffer, "GL_OES_mapbuffer");
    CheckExtension(MapBufferRange, "GL_EXT_map_buffer_range");
#endif
    CheckExtension(UintIndices, "GL_OES_element_index_uint");
  }
  else
  {
#ifdef OMIM_OS_ANDROID
    SetExtension(MapBuffer, false);
    SetExtension(MapBufferRange, false);
#else
    SetExtension(MapBuffer, true);
    SetExtension(MapBufferRange, true);
#endif
    SetExtension(VertexArrayObject, true);
    SetExtension(UintIndices, true);
  }
#elif defined(OMIM_OS_LINUX)
  SetExtension(MapBuffer, true);
  SetExtension(UintIndices, true);
  if (apiVersion == dp::ApiVersion::OpenGLES2)
  {
    SetExtension(VertexArrayObject, true);
    SetExtension(MapBuffer, true);
    SetExtension(MapBufferRange, true);
  }
  else // OpenGLES3 branch
  {
    SetExtension(VertexArrayObject, true);
    SetExtension(MapBufferRange, true);
  }
#elif defined(OMIM_OS_WINDOWS)
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

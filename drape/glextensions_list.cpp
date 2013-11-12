#include "glextensions_list.hpp"
#include "glfunctions.hpp"

#include "../base/assert.hpp"

GLExtensionsList::GLExtensionsList()
{
  m_supportMap[VertexArrayObject] = GLFunctions::glHasExtension("GL_OES_vertex_array_object");
}

GLExtensionsList & GLExtensionsList::Instance()
{
  static GLExtensionsList extList;
  return extList;
}

bool GLExtensionsList::IsSupported(const ExtensionName & extName)
{
  ASSERT(m_supportMap.find(extName) != m_supportMap.end(), ("Not all used extensions is checked"));
  return m_supportMap[extName];
}

#include "drape/gl_extensions_list.hpp"
#include "drape/gl_functions.hpp"

#include "base/assert.hpp"

#include "std/target_os.hpp"

namespace dp
{
void GLExtensionsList::Init()
{
#ifdef OMIM_OS_ANDROID
  // NOTE: MapBuffer/MapBufferRange are disabled by performance reasons according to
  // https://github.com/organicmaps/organicmaps/commit/d72ab7c8cd8be0eb5a622d9d33ae943b391d5707
  SetExtension(MapBuffer, false);
#else
  SetExtension(MapBuffer, true);
#endif
}

bool GLExtensionsList::IsSupported(ExtensionName extName) const
{
  ASSERT_LESS(extName, ExtensionCount, ("Not all used extensions are checked"));
  return m_supportedMap[extName];
}

void GLExtensionsList::CheckExtension(ExtensionName enumName, std::string const & extName)
{
  ASSERT_LESS(enumName, ExtensionCount, ());
  m_supportedMap[enumName] = GLFunctions::glHasExtension(extName);
}

void GLExtensionsList::SetExtension(ExtensionName enumName, bool isSupported)
{
  ASSERT_LESS(enumName, ExtensionCount, ());
  m_supportedMap[enumName] = isSupported;
}
}  // namespace dp

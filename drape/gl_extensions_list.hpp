#pragma once

#include "drape/drape_global.hpp"

#include "base/macros.hpp"

#include <map>
#include <string>

namespace dp
{
class GLExtensionsList
{
public:
  enum ExtensionName
  {
    VertexArrayObject,
    MapBuffer,
    UintIndices,
    MapBufferRange
  };

  GLExtensionsList() = default;
  void Init(dp::ApiVersion apiVersion);
  bool IsSupported(ExtensionName extName) const;

private:

  void CheckExtension(ExtensionName enumName, std::string const & extName);
  void SetExtension(ExtensionName enumName, bool isSupported);

  std::map<ExtensionName, bool> m_supportedMap;

  DISALLOW_COPY_AND_MOVE(GLExtensionsList);
};
}  // namespace dp

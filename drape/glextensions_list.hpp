#pragma once

#include "../std/noncopyable.hpp"
#include "../std/map.hpp"

class GLExtensionsList : private noncopyable
{
public:
  enum ExtensionName
  {
    VertexArrayObject
  };

  static GLExtensionsList & Instance();
  bool IsSupported(const ExtensionName & extName);

private:
  GLExtensionsList();
  map<ExtensionName, bool> m_supportMap;
};

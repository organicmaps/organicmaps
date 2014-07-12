#pragma once

#include "../std/noncopyable.hpp"

class GLExtensionsList : private noncopyable
{
public:
  enum ExtensionName
  {
    VertexArrayObject,
    TextureNPOT,
    RequiredInternalFormat,
    MapBuffer
  };

  static GLExtensionsList & Instance();
  bool IsSupported(ExtensionName const & extName) const;

private:
  GLExtensionsList();
  ~GLExtensionsList();

private:
  class Impl;
  Impl * m_impl;
};

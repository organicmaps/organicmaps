#pragma once

#include "../std/noncopyable.hpp"

class GLExtensionsList : private noncopyable
{
public:
  enum ExtensionName
  {
    VertexArrayObject,
    TextureNPOT,
    RequiredInternalFormat
  };

  static GLExtensionsList & Instance();
  bool IsSupported(const ExtensionName & extName) const;

private:
  GLExtensionsList();
  ~GLExtensionsList();

private:
  class Impl;
  Impl * m_impl;
};

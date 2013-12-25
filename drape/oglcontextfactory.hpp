#pragma once

#include "oglcontext.hpp"

class OGLContextFactory
{
public:
  virtual ~OGLContextFactory() {}
  virtual OGLContext * getDrawContext() = 0;
  virtual OGLContext * getResourcesUploadContext() = 0;
};

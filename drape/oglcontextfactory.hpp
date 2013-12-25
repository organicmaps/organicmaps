#pragma once

#include "oglcontext.hpp"

class OGLContextFactory
{
  virtual OGLContext * getDrawContext() = 0;
  virtual OGLContext * getResourcesUploadContext() = 0;
};

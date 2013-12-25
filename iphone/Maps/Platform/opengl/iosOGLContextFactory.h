#pragma once

#import "iosOGLContext.h"
#import "../../../../drape/oglcontextfactory.hpp"

class iosOGLContextFactory: public OGLContextFactory
{
  iosOGLContextFactory(CAEAGLLayer * layer);

  virtual OGLContext * getDrawContext();
  virtual OGLContext * getResourcesUploadContext();

private:
  CAEAGLLayer * m_layer;
  iosOGLContext * m_drawContext;
  iosOGLContext * m_uploadContext;
};
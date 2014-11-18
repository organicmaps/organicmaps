#pragma once

#import "iosOGLContext.h"
#import "../../../../drape/oglcontextfactory.hpp"

class iosOGLContextFactory: public dp::OGLContextFactory
{
public:
  iosOGLContextFactory(CAEAGLLayer * layer);
  ~iosOGLContextFactory();

  virtual dp::OGLContext * getDrawContext();
  virtual dp::OGLContext * getResourcesUploadContext();

private:
  CAEAGLLayer * m_layer;
  iosOGLContext * m_drawContext;
  iosOGLContext * m_uploadContext;
};
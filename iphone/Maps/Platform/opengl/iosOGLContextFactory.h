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

  virtual bool isDrawContextCreated() const;
  virtual bool isUploadContextCreated() const;

private:
  CAEAGLLayer * m_layer;
  iosOGLContext * m_drawContext;
  iosOGLContext * m_uploadContext;
};
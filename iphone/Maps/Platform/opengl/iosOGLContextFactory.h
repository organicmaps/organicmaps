#pragma once

#import "iosOGLContext.h"
#import "../../../../drape/oglcontextfactory.hpp"

#include "std/condition_variable.hpp"
#include "std/mutex.hpp"

class iosOGLContextFactory: public dp::OGLContextFactory
{
public:
  iosOGLContextFactory(CAEAGLLayer * layer);
  ~iosOGLContextFactory();

  dp::OGLContext * getDrawContext() override;
  dp::OGLContext * getResourcesUploadContext() override;

  bool isDrawContextCreated() const override;
  bool isUploadContextCreated() const override;
  
  void waitForInitialization() override;
  
  void setPresentAvailable(bool available);

private:
  CAEAGLLayer * m_layer;
  iosOGLContext * m_drawContext;
  iosOGLContext * m_uploadContext;
  
  bool m_isInitialized;
  condition_variable m_initializationCondition;
  mutex m_initializationMutex;
};

#pragma once

#import "iosOGLContext.h"

#include "drape/oglcontextfactory.hpp"
#include "drape/drape_global.hpp"

#include "std/condition_variable.hpp"
#include "std/mutex.hpp"

class iosOGLContextFactory: public dp::OGLContextFactory
{
public:
  iosOGLContextFactory(CAEAGLLayer * layer, dp::ApiVersion apiVersion);
  ~iosOGLContextFactory();

  dp::OGLContext * getDrawContext() override;
  dp::OGLContext * getResourcesUploadContext() override;

  bool isDrawContextCreated() const override;
  bool isUploadContextCreated() const override;
  
  void waitForInitialization(dp::OGLContext * context) override;
  
  void setPresentAvailable(bool available);

private:
  CAEAGLLayer * m_layer;
  dp::ApiVersion m_apiVersion;
  iosOGLContext * m_drawContext;
  iosOGLContext * m_uploadContext;
  
  bool m_isInitialized;
  size_t m_initializationCounter;
  bool m_presentAvailable;
  condition_variable m_initializationCondition;
  mutex m_initializationMutex;
};

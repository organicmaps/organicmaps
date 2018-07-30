#pragma once

#import "iosOGLContext.h"

#include "drape/graphic_context_factory.hpp"
#include "drape/drape_global.hpp"

#include "std/condition_variable.hpp"
#include "std/mutex.hpp"

class iosOGLContextFactory: public dp::GraphicContextFactory
{
public:
  iosOGLContextFactory(CAEAGLLayer * layer, dp::ApiVersion apiVersion);
  ~iosOGLContextFactory();

  dp::GraphicContext * GetDrawContext() override;
  dp::GraphicContext * GetResourcesUploadContext() override;

  bool IsDrawContextCreated() const override;
  bool IsUploadContextCreated() const override;
  
  void WaitForInitialization(dp::GraphicContext * context) override;
  
  void SetPresentAvailable(bool available) override;

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

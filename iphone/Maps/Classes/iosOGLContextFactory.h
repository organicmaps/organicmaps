#pragma once

#import "iosOGLContext.h"

#include "drape/graphics_context_factory.hpp"
#include "drape/drape_global.hpp"

#include <condition_variable>
#include <mutex>

class iosOGLContextFactory: public dp::GraphicsContextFactory
{
public:
  iosOGLContextFactory(CAEAGLLayer * layer, dp::ApiVersion apiVersion, bool presentAvailable);
  ~iosOGLContextFactory();

  dp::GraphicsContext * GetDrawContext() override;
  dp::GraphicsContext * GetResourcesUploadContext() override;

  bool IsDrawContextCreated() const override;
  bool IsUploadContextCreated() const override;
  
  void WaitForInitialization(dp::GraphicsContext * context) override;
  
  void SetPresentAvailable(bool available) override;

private:
  CAEAGLLayer * m_layer;
  dp::ApiVersion m_apiVersion;
  iosOGLContext * m_drawContext;
  iosOGLContext * m_uploadContext;
  
  bool m_isInitialized;
  size_t m_initializationCounter;
  bool m_presentAvailable;
  std::condition_variable m_initializationCondition;
  std::mutex m_initializationMutex;
};

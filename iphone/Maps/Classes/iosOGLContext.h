#pragma once

#include "drape/drape_global.hpp"
#include "drape/gl_includes.hpp"
#include "drape/oglcontext.hpp"

#import <QuartzCore/CAEAGLLayer.h>

#include <atomic>

class iosOGLContext : public dp::OGLContext
{
public:
  iosOGLContext(CAEAGLLayer * layer, dp::ApiVersion apiVersion, iosOGLContext * contextToShareWith,
                bool needBuffers = false);
  ~iosOGLContext();

  void MakeCurrent() override;
  void Present() override;
  void SetFramebuffer(ref_ptr<dp::BaseFramebuffer> framebuffer) override;
  void Resize(uint32_t w, uint32_t h) override;
  void SetPresentAvailable(bool available) override;

private:
  dp::ApiVersion m_apiVersion;
  CAEAGLLayer * m_layer;
  EAGLContext * m_nativeContext;

  void InitBuffers();
  void DestroyBuffers();

  //{@ Buffers
  bool m_needBuffers;
  bool m_hasBuffers;

  GLuint m_renderBufferId;
  GLuint m_depthBufferId;
  GLuint m_frameBufferId;
  //@} buffers

  std::atomic<bool> m_presentAvailable;
};

#pragma once

#include "drape/drape_global.hpp"
#include "drape/oglcontext.hpp"
#include "drape/glIncludes.hpp"

#import <QuartzCore/CAEAGLLayer.h>

#include <atomic>

class iosOGLContext : public dp::OGLContext
{
public:
  iosOGLContext(CAEAGLLayer * layer, dp::ApiVersion apiVersion,
                iosOGLContext * contextToShareWith, bool needBuffers = false);
  ~iosOGLContext();

  virtual void makeCurrent();
  virtual void present();
  virtual void setDefaultFramebuffer();
  virtual void resize(int w, int h);
  
  void setPresentAvailable(bool available);

private:
  dp::ApiVersion m_apiVersion;
  CAEAGLLayer * m_layer;
  EAGLContext * m_nativeContext;

  void initBuffers();
  void destroyBuffers();

  //{@ Buffers
  bool m_needBuffers;
  bool m_hasBuffers;

  GLuint m_renderBufferId;
  GLuint m_depthBufferId;
  GLuint m_frameBufferId;
  //@} buffers
  
  std::atomic<bool> m_presentAvailable;
};

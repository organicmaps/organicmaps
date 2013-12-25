#pragma once

#import "../../../../drape/oglcontext.hpp"

#import <QuartzCore/CAEAGLLayer.h>
#import <OpenGLES/ES2/gl.h>
#import <OpenGLES/ES2/glext.h>

class iosOGLContext : public OGLContext
{
public:
  iosOGLContext(CAEAGLLayer * layer, iosOGLContext * contextToShareWith, bool needBuffers = false);
  ~iosOGLContext();

  virtual void makeCurrent();
  virtual void present();
  virtual void setDefaultFramebuffer();

private:
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
};
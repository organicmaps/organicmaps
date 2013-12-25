#pragma once

class OGLContext
{
public:
  virtual ~OGLContext() {}
  virtual void present() = 0;
  virtual void makeCurrent() = 0;
  virtual void setDefaultFramebuffer() = 0;
};

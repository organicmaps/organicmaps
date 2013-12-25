#pragma once

class OGLContext
{
public:
  virtual void present() = 0;
  virtual void makeCurrent() = 0;
};

#pragma once

namespace dp
{

class OGLContext
{
public:
  virtual ~OGLContext() {}
  virtual void present() = 0;
  virtual void makeCurrent() = 0;
  virtual void setDefaultFramebuffer() = 0;
  /// @ param w, h - pixel size of render target (logical size * visual scale)
  virtual void resize(int /*w*/, int /*h*/) {}
  virtual int additionClearFlags() { return 0; }
  virtual void setRenderingEnabled(bool enabled) {}
};

} // namespace dp

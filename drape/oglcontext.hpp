#pragma once

namespace dp
{
class OGLContext
{
public:
  virtual ~OGLContext() {}
  virtual void present() = 0;
  virtual void makeCurrent() = 0;
  virtual void doneCurrent() {}
  virtual void setDefaultFramebuffer() = 0;
  // w, h - pixel size of render target (logical size * visual scale).
  virtual void resize(int /*w*/, int /*h*/) {}
  virtual void setRenderingEnabled(bool /*enabled*/) {}
  virtual void setPresentAvailable(bool /*available*/) {}
  virtual bool validate() { return true; }
};
}  // namespace dp

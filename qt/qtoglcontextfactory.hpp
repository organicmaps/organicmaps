#pragma once

#include "drape/oglcontextfactory.hpp"
#include "qt/qtoglcontext.hpp"

#include <QtGui/QOffscreenSurface>

class QtOGLContextFactory : public dp::OGLContextFactory
{
public:
  using TRegisterThreadFn = QtRenderOGLContext::TRegisterThreadFn;
  using TSwapFn = QtRenderOGLContext::TSwapFn;

  QtOGLContextFactory(QOpenGLContext * renderContext, QThread * thread,
                      TRegisterThreadFn const & regFn, TSwapFn const & swapFn);
  ~QtOGLContextFactory();

  void shutDown() { m_drawContext->shutDown(); }

  virtual dp::OGLContext * getDrawContext();
  virtual dp::OGLContext * getResourcesUploadContext();

protected:
  virtual bool isDrawContextCreated() const { return m_drawContext != nullptr; }
  virtual bool isUploadContextCreated() const { return m_uploadContext != nullptr; }

private:
  QtRenderOGLContext * m_drawContext;
  QtUploadOGLContext * m_uploadContext;
  QOffscreenSurface * m_uploadThreadSurface;
};

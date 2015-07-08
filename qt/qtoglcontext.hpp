#pragma once

#include "drape/oglcontext.hpp"

#include "std/function.hpp"
#include "std/atomic.hpp"

#include <QtGui/QWindow>
#include <QtGui/QOpenGLContext>

#include <QtCore/QThread>

class QtRenderOGLContext : public dp::OGLContext
{
public:
  using TRegisterThreadFn = function<void (QThread * thread)>;
  using TSwapFn = function<void ()>;

  QtRenderOGLContext(QOpenGLContext * nativeContext, QThread * guiThread,
                     TRegisterThreadFn const & regFn, TSwapFn const & swapFn);

  void present() override;
  void makeCurrent() override;
  void doneCurrent() override;
  void setDefaultFramebuffer() override;

  void shutDown();

  QOpenGLContext * getNativeContext() { return m_ctx; }

private:
  void MoveContextOnGui();

private:
  QSurface * m_surface;
  QOpenGLContext * m_ctx;
  QThread * m_guiThread;
  TRegisterThreadFn m_regFn;
  TSwapFn m_swapFn;
  bool m_isRegistered;
  atomic<bool> m_shutedDown;
};

class QtUploadOGLContext: public dp::OGLContext
{
public:
  QtUploadOGLContext(QSurface * surface, QOpenGLContext * contextToShareWith);
  ~QtUploadOGLContext();

  virtual void present();
  virtual void makeCurrent();
  virtual void setDefaultFramebuffer();

private:
  QOpenGLContext * m_nativeContext;
  QSurface * m_surface;
};

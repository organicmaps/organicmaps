#pragma once

#include "drape/oglcontext.hpp"
#include "std/mutex.hpp"

#include <QtGui/QOffscreenSurface>
#include <QtGui/QOpenGLFramebufferObject>
#include <QtGui/QOpenGLContext>

class QtRenderOGLContext : public dp::OGLContext
{
public:
  QtRenderOGLContext(QOpenGLContext * rootContext, QOffscreenSurface * surface);
  ~QtRenderOGLContext();

  void present() override;
  void makeCurrent() override;
  void doneCurrent() override;
  void setDefaultFramebuffer() override;
  void resize(int w, int h) override;

  void lockFrame();
  GLuint getTextureHandle() const;
  QRectF const & getTexRect() const;
  void unlockFrame();

private:
  QOffscreenSurface * m_surface = nullptr;
  QOpenGLContext * m_ctx = nullptr;

  QOpenGLFramebufferObject * m_frontFrame = nullptr;
  QOpenGLFramebufferObject * m_backFrame = nullptr;
  QRectF m_texRect = QRectF(0.0, 0.0, 0.0, 0.0);

  mutex m_lock;
  bool m_resizeLock = false;
};

class QtUploadOGLContext: public dp::OGLContext
{
public:
  QtUploadOGLContext(QOpenGLContext * rootContext, QOffscreenSurface * surface);
  ~QtUploadOGLContext();

  void present() override;
  void makeCurrent() override;
  void doneCurrent() override;
  void setDefaultFramebuffer() override;

private:
  QOpenGLContext * m_ctx = nullptr;
  QOffscreenSurface * m_surface = nullptr;
};

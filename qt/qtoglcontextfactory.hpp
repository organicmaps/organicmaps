#pragma once

#include "drape/oglcontextfactory.hpp"
#include "qt/qtoglcontext.hpp"

#include <QtGui/QOpenGLContext>
#include <QtGui/QOpenGLFramebufferObject>

class QtOGLContextFactory : public dp::OGLContextFactory
{
public:
  QtOGLContextFactory(QOpenGLContext * rootContext);
  ~QtOGLContextFactory();

  bool LockFrame();
  GLuint GetTextureHandle() const;
  QRectF const & GetTexRect() const;
  void UnlockFrame();

  virtual dp::OGLContext * getDrawContext();
  virtual dp::OGLContext * getResourcesUploadContext();

protected:
  virtual bool isDrawContextCreated() const { return m_drawContext != nullptr; }
  virtual bool isUploadContextCreated() const { return m_uploadContext != nullptr; }

  QOffscreenSurface * createSurface();

private:
  QOpenGLContext * m_rootContext;
  QtRenderOGLContext * m_drawContext;
  QOffscreenSurface * m_drawSurface;
  QtUploadOGLContext * m_uploadContext;
  QOffscreenSurface * m_uploadSurface;
};

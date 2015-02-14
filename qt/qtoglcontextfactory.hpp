#pragma once

#include "drape/oglcontextfactory.hpp"
#include "qt/qtoglcontext.hpp"

#include <QtGui/QWindow>

class QtOGLContextFactory : public dp::OGLContextFactory
{
public:
  QtOGLContextFactory(QWindow * surface);
  ~QtOGLContextFactory();

  virtual dp::OGLContext * getDrawContext();
  virtual dp::OGLContext * getResourcesUploadContext();

protected:
  virtual bool isDrawContextCreated() const { return m_drawContext != nullptr; }
  virtual bool isUploadContextCreated() const { return m_uploadContext != nullptr; }

private:
  QWindow * m_surface;
  QtOGLContext * m_drawContext;
  QtOGLContext * m_uploadContext;
};

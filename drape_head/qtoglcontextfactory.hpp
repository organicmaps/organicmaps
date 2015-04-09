#pragma once

#include "drape/oglcontextfactory.hpp"
#include "drape_head/qtoglcontext.hpp"

#include <QtGui/QWindow>

class QtOGLContextFactory : public dp::OGLContextFactory
{
public:
  QtOGLContextFactory(QWindow * surface);
  ~QtOGLContextFactory();

  virtual dp::OGLContext * getDrawContext();
  virtual dp::OGLContext * getResourcesUploadContext();

private:
  QWindow * m_surface;
  QtOGLContext * m_drawContext;
  QtOGLContext * m_uploadContext;
};

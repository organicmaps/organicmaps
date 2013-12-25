#pragma once

#include "../../drape/oglcontextfactory.hpp"
#include "qtoglcontext.hpp"

#include <QtGui/QWindow>

class QtOGLContextFactory
{
public:
  QtOGLContextFactory(QWindow * surface);

  virtual OGLContext * getDrawContext();
  virtual OGLContext * getResourcesUploadContext();

private:
  QWindow * m_surface;
  QtOGLContext * m_drawContext;
  QtOGLContext * m_uploadContext;
};

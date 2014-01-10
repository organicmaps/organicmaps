#pragma once

#include "../base/mutex.hpp"

#include "../../drape/oglcontextfactory.hpp"
#include "qtoglcontext.hpp"

#include <QtGui/QWindow>

class QtOGLContextFactory : public OGLContextFactory
{
public:
  QtOGLContextFactory(QWindow * surface);
  ~QtOGLContextFactory();

  virtual OGLContext * getDrawContext();
  virtual OGLContext * getResourcesUploadContext();

private:
  QWindow * m_surface;
  QtOGLContext * m_drawContext;
  QtOGLContext * m_uploadContext;

  threads::Mutex m_mutex;
};

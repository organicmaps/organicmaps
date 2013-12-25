#pragma once

#include "../drape/oglcontext.hpp"

#include <QtGui/QWindow>
#include <QtGui/QOpenGLContext>

class QtOGLContext: public OGLContext
{
public:
  QtOGLContext(QWindow * surface);
  QtOGLContext(QWindow *surface, QtOGLContext * contextToShareWith);

  virtual void present();
  virtual void makeCurrent();

private:
  QOpenGLContext * m_nativeContext;
  QWindow * m_surface;
  bool m_isContextCreated;
};

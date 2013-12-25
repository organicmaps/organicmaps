#pragma once

#include "../drape/oglcontext.hpp"

#include <QtGui/QWindow>
#include <QtGui/QOpenGLContext>

class QtOGLContext: public OGLContext
{
public:
  QtOGLContext(QWindow * surface, QtOGLContext * contextToShareWith);
  ~QtOGLContext();

  virtual void present();
  virtual void makeCurrent();
  virtual void setDefaultFramebuffer();

private:
  QOpenGLContext * m_nativeContext;
  QWindow * m_surface;
  bool m_isContextCreated;
};

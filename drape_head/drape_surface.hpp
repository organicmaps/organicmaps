#pragma once

#include "qtoglcontextfactory.hpp"
#include <QtGui/QWindow>

class DrapeSurface : public QWindow
{
public:
  DrapeSurface();
  ~DrapeSurface();

protected:
  void exposeEvent(QExposeEvent * e);

private:
  void CreateEngine() {}

private:
  QtOGLContextFactory * m_contextFactory;
};

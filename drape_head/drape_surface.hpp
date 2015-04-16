#pragma once

#include "drape_head/qtoglcontextfactory.hpp"
#include "drape_head/testing_engine.hpp"

#include <QtGui/QWindow>

class DrapeSurface : public QWindow
{
  Q_OBJECT

public:
  DrapeSurface();
  ~DrapeSurface();

protected:
  void exposeEvent(QExposeEvent * e);

private:
  void CreateEngine();

  Q_SLOT void sizeChanged(int);

private:
  typedef drape_ptr<dp::OGLContextFactory> TContextFactoryPtr;
  typedef drape_ptr<df::TestingEngine> TEnginePrt;
  TContextFactoryPtr m_contextFactory;
  TEnginePrt m_drapeEngine;
};

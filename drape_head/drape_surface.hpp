#pragma once

#include "qtoglcontextfactory.hpp"

#include "../drape/batcher.hpp"
#include "../drape/gpu_program_manager.hpp"
#include "../drape/uniform_values_storage.hpp"

#if defined(USE_TESTING_ENGINE)
  #include "testing_engine.hpp"
  #define DrapeEngine TestingEngine
#else
  #include "../drape_frontend/drape_engine.hpp"
#endif

#include <QtGui/QWindow>
#include <QtCore/QTimerEvent>

class DrapeSurface : public QWindow
{
  Q_OBJECT

public:
  DrapeSurface();
  ~DrapeSurface();

protected:
  void exposeEvent(QExposeEvent * e);
  void mousePressEvent(QMouseEvent * e);
  void mouseMoveEvent(QMouseEvent * e);
  void mouseReleaseEvent(QMouseEvent * e);
  void wheelEvent(QWheelEvent * e);

private:
  void CreateEngine();

  Q_SLOT void sizeChanged(int);

private:
  m2::PointF GetDevicePosition(const QPoint & p);

  bool m_dragState;

private:
  MasterPointer<OGLContextFactory> m_contextFactory;
  MasterPointer<df::DrapeEngine>   m_drapeEngine;
};

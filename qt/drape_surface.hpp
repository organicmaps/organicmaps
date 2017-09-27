#pragma once

#include "qtoglcontextfactory.hpp"

#include "map/framework.hpp"
#include "drape_frontend/drape_engine.hpp"

#include <QtGui/QWindow>

namespace qt
{

class DrapeSurface : public QWindow
{
  Q_OBJECT

public:
  DrapeSurface();
  ~DrapeSurface();

  void LoadState();
  void SaveState();

  Framework & GetFramework() { return m_framework; }

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
  m2::PointF GetDevicePosition(QPoint const & p);

  bool m_dragState;

private:
  Framework m_framework;
  dp::MasterPointer<dp::OGLContextFactory> m_contextFactory;
};

} // namespace qt

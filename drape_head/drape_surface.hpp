#pragma once

#include "drape_head/testing_engine.hpp"

#include <QtGui/QOpenGLWindow>
#include <QtCore/QTimer>

class DrapeSurface : public QOpenGLWindow
{
  Q_OBJECT

public:
  DrapeSurface();
  ~DrapeSurface();

protected:
  void initializeGL() override;
  void paintGL() override;
  void resizeGL(int w, int h) override;

private:
  void CreateEngine();

  drape_ptr<df::TestingEngine> m_drapeEngine;
  QTimer m_timer;
};

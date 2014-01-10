#pragma once

#include "qtoglcontextfactory.hpp"

#include "../drape/batcher.hpp"
#include "../drape/gpu_program_manager.hpp"
#include "../drape/uniform_values_storage.hpp"
#include "../drape_frontend/drape_engine.hpp"

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
  void timerEvent(QTimerEvent * e);

private:
  void CreateEngine();

  Q_SLOT void sizeChanged(int);

private:

  int m_timerID;

private:
  MasterPointer<OGLContextFactory> m_contextFactory;
  MasterPointer<df::DrapeEngine>   m_drapeEngine;
};

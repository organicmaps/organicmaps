#pragma once

#include "drape_head/qtoglcontextfactory.hpp"

#include "map/feature_vec_model.hpp"
#include "map/navigator.hpp"

#include "drape/batcher.hpp"
#include "drape/gpu_program_manager.hpp"
#include "drape/uniform_values_storage.hpp"

//#define USE_TESTING_ENGINE
#if defined(USE_TESTING_ENGINE)
#include "drape_head/testing_engine.hpp"
#define DrapeEngine TestingEngine
#else
#include "drape_frontend/drape_engine.hpp"
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
  void UpdateCoverage();

  Q_SLOT void sizeChanged(int);

private:
  m2::PointF GetDevicePosition(QPoint const & p);

  bool m_dragState;

  model::FeaturesFetcher m_model;
  Navigator m_navigator;

private:
  typedef dp::MasterPointer<dp::OGLContextFactory> TContextFactoryPtr;
  typedef dp::MasterPointer<df::DrapeEngine> TEnginePrt;
  TContextFactoryPtr m_contextFactory;
  TEnginePrt m_drapeEngine;
};

#pragma once

#include "drape/pointers.hpp"
#include "drape_frontend/gui/skin.hpp"
#include "drape_frontend/user_event_stream.hpp"
#include "qt/qt_common/qtoglcontextfactory.hpp"

#include <QtCore/QTimer>
#include <QtWidgets/QOpenGLWidget>

#include <memory>

class Framework;
class QMouseEvent;
class QWidget;
class ScreenBase;

namespace qt
{
namespace common
{
class ScaleSlider;

class MapWidget : public QOpenGLWidget
{
  Q_OBJECT

public:
  MapWidget(Framework & framework, QWidget * parent);
  ~MapWidget() override;

  void BindHotkeys(QWidget & parent);
  void BindSlider(ScaleSlider & slider);
  void CreateEngine();

public Q_SLOTS:
  void ScalePlus();
  void ScaleMinus();
  void ScalePlusLight();
  void ScaleMinusLight();

  void ScaleChanged(int action);
  void SliderPressed();
  void SliderReleased();

public:
  Q_SIGNAL void BeforeEngineCreation();

protected:
  enum class SliderState
  {
    Pressed,
    Released
  };

  int L2D(int px) const { return px * m_ratio; }
  m2::PointD GetDevicePoint(QMouseEvent * e) const;
  df::Touch GetTouch(QMouseEvent * e) const;
  df::TouchEvent GetTouchEvent(QMouseEvent * e, df::TouchEvent::ETouchType type) const;
  df::Touch GetSymmetrical(df::Touch const & touch) const;

  void UpdateScaleControl();

  // QOpenGLWidget overrides:
  void initializeGL() override;
  void paintGL() override;
  void resizeGL(int width, int height) override;

  void mouseDoubleClickEvent(QMouseEvent * e) override;
  void mousePressEvent(QMouseEvent * e) override;
  void mouseMoveEvent(QMouseEvent * e) override;
  void mouseReleaseEvent(QMouseEvent * e) override;
  void wheelEvent(QWheelEvent * e) override;

  Framework & m_framework;
  ScaleSlider * m_slider;
  SliderState m_sliderState;

  qreal m_ratio;
  drape_ptr<QtOGLContextFactory> m_contextFactory;
  std::unique_ptr<gui::Skin> m_skin;

  std::unique_ptr<QTimer> m_updateTimer;
};
}  // namespace common
}  // namespace qt

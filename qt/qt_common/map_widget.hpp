#pragma once

#include "drape/pointers.hpp"
#include "drape_frontend/gui/skin.hpp"
#include "drape_frontend/user_event_stream.hpp"

#include "kml/type_utils.hpp"

#include "qt/qt_common/qtoglcontextfactory.hpp"

#include <QtCore/QTimer>
#include <QtWidgets/QOpenGLWidget>

#include <memory>

class Framework;
class QMouseEvent;
class QWidget;
class ScreenBase;
class QOpenGLShaderProgram;
class QOpenGLVertexArrayObject;
class QOpenGLBuffer;

namespace qt
{
namespace common
{
class ScaleSlider;

class MapWidget : public QOpenGLWidget
{
  Q_OBJECT

public:
  MapWidget(Framework & framework, bool apiOpenGLES3, QWidget * parent);
  ~MapWidget() override;

  void BindHotkeys(QWidget & parent);
  void BindSlider(ScaleSlider & slider);
  void CreateEngine();

signals:
  void OnContextMenuRequested(QPoint const & p);

public slots:
  void ScalePlus();
  void ScaleMinus();
  void ScalePlusLight();
  void ScaleMinusLight();

  void ScaleChanged(int action);
  void SliderPressed();
  void SliderReleased();

  void AntialiasingOn();
  void AntialiasingOff();

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
  void Build();

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
  bool m_apiOpenGLES3;
  ScaleSlider * m_slider;
  SliderState m_sliderState;
  kml::MarkGroupId m_bookmarksCategoryId = 0;

  qreal m_ratio;
  drape_ptr<QtOGLContextFactory> m_contextFactory;
  std::unique_ptr<gui::Skin> m_skin;

  std::unique_ptr<QTimer> m_updateTimer;

  unique_ptr<QOpenGLShaderProgram> m_program;
  unique_ptr<QOpenGLVertexArrayObject> m_vao;
  unique_ptr<QOpenGLBuffer> m_vbo;
};
}  // namespace common
}  // namespace qt

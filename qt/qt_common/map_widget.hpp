#pragma once

#include <QWidget>

#include "drape_frontend/user_event_stream.hpp"
#include "qt/qt_common/renderer/base/renderer_window.hpp"

class Framework;
class QMouseEvent;
class ScreenBase;

namespace qt::common
{
class ScaleSlider;

class MapWidget : public QWidget
{
  Q_OBJECT

public:
  MapWidget(Framework & framework, QWidget * parent);
  ~MapWidget() override;

  void BindHotkeys(QWidget & parent);
  void BindSlider(ScaleSlider & slider);

signals:
  void OnContextMenuRequested(QPoint const & p);
  void BeforeEngineCreation();
  void AfterEngineCreation();

public slots:
  void ScalePlus();
  void ScaleMinus();
  void ScalePlusLight();
  void ScaleMinusLight();
  void MoveRight();
  void MoveRightSmooth();
  void MoveLeft();
  void MoveLeftSmooth();
  void MoveUp();
  void MoveUpSmooth();
  void MoveDown();
  void MoveDownSmooth();

  void ScaleChanged(int action);
  void SliderPressed();
  void SliderReleased();

  void AntialiasingOn();
  void AntialiasingOff();

  void SetRenderingApi(dp::ApiVersion apiVersion);

protected slots:
  virtual void OnBeforeEngineCreation();
  virtual void OnAfterEngineCreation();

protected:
  enum class SliderState
  {
    Pressed,
    Released
  };

  int L2D(int px) const;
  m2::PointD GetDevicePoint(QMouseEvent const * e) const;
  df::Touch GetDfTouchFromQMouseEvent(QMouseEvent const * e) const;
  df::TouchEvent GetDfTouchEventFromQMouseEvent(QMouseEvent const * e, df::TouchEvent::ETouchType type) const;
  df::Touch GetSymmetrical(df::Touch const & touch) const;

  void mouseDoubleClickEvent(QMouseEvent * e) override;
  void mousePressEvent(QMouseEvent * e) override;
  void mouseMoveEvent(QMouseEvent * e) override;
  void mouseReleaseEvent(QMouseEvent * e) override;
  void wheelEvent(QWheelEvent * e) override;

  void UpdateScaleControl();
  void ShowInfoPopup(QMouseEvent * e, m2::PointD const & pt);

  void OnViewportChanged(ScreenBase const & screen);

protected:
  Framework & m_framework;
  ScaleSlider * m_slider;
  SliderState m_sliderState;

  renderer::base::RendererWindow * m_rendererWindow;
  QWidget * m_windowContainer;
};

}  // namespace qt::common

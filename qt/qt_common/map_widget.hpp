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

protected:
  enum class SliderState
  {
    Pressed,
    Released
  };

  int L2D(int const px) const { return m_rendererWindow->L2D(px); }
  m2::PointD GetDevicePoint(QMouseEvent const * const e) const { return m_rendererWindow->GetDevicePoint(e); }
  df::Touch GetDfTouchFromQMouseEvent(QMouseEvent const * const e) const
  {
    return m_rendererWindow->GetDfTouchFromQMouseEvent(e);
  }
  df::TouchEvent GetDfTouchEventFromQMouseEvent(QMouseEvent const * const e,
                                                df::TouchEvent::ETouchType const type) const
  {
    return m_rendererWindow->GetDfTouchEventFromQMouseEvent(e, type);
  }
  df::Touch GetSymmetrical(df::Touch const & touch) const { return m_rendererWindow->GetSymmetrical(touch); }

  void UpdateScaleControl();
  void ShowInfoPopup(QMouseEvent * e, m2::PointD const & pt);

  void OnViewportChanged(ScreenBase const & screen);

  Framework & m_framework;
  ScaleSlider * m_slider;
  SliderState m_sliderState;

  renderer::base::RendererWindow * m_rendererWindow;
  QWidget * m_windowContainer;
};

}  // namespace qt::common

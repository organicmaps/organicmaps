#pragma once

#include <QMouseEvent>
#include <QWindow>

#include <memory>

#include "map/framework.hpp"

namespace qt::common::renderer::base
{
class RendererWindow : public QWindow
{
  Q_OBJECT

public:
  explicit RendererWindow(Framework & framework, SurfaceType surfaceType, QSurfaceFormat const & surfaceFormat,
                          QWindow * parent = nullptr);
  ~RendererWindow() override;

  int L2D(int px) const;
  m2::PointD GetDevicePoint(QMouseEvent const * e) const;
  df::Touch GetDfTouchFromQMouseEvent(QMouseEvent const * e) const;
  df::TouchEvent GetDfTouchEventFromQMouseEvent(QMouseEvent const * e, df::TouchEvent::ETouchType type) const;
  df::Touch GetSymmetrical(df::Touch const & touch) const;

signals:
  void OnBeforeEngineCreation();
  void OnViewportChanged(ScreenBase const & screen);
  void OnContextMenuRequested(QPoint const & p);

protected:
  void CreateDrapeEngine(dp::ApiVersion apiVersion, ref_ptr<dp::GraphicsContextFactory> contextFactory);
  void OnResize(int w, int h) const;

  virtual void Render() = 0;

  bool event(QEvent * e) override;
  void mouseDoubleClickEvent(QMouseEvent * e) override;
  void mousePressEvent(QMouseEvent * e) override;
  void mouseMoveEvent(QMouseEvent * e) override;
  void mouseReleaseEvent(QMouseEvent * e) override;
  void wheelEvent(QWheelEvent * e) override;

  void exposeEvent(QExposeEvent * e) override;
  void resizeEvent(QResizeEvent * e) override;

  Framework & m_framework;
  float m_ratio;

private:
  std::unique_ptr<gui::Skin> m_skin;
};
}  // namespace qt::common::renderer::base

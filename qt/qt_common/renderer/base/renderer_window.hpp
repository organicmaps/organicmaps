#pragma once

#include <QPointer>
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

  dp::ApiVersion GetApiVersion() const;
  float GetRatio() const { return m_ratio; }

  void SetEventReceiver(QObject * receiver) { m_eventReceiver = receiver; }

signals:
  void OnBeforeEngineCreation();
  void OnAfterEngineCreation();
  void OnViewportChanged(ScreenBase const & screen);

protected:
  virtual void Render() = 0;

  void CreateDrapeEngine(dp::ApiVersion apiVersion, ref_ptr<dp::GraphicsContextFactory> contextFactory);
  void OnResize(int w, int h) const;

  bool event(QEvent * e) override;
  void exposeEvent(QExposeEvent * e) override;
  void resizeEvent(QResizeEvent * e) override;

  Framework & m_framework;
  float m_ratio;

private:
  std::unique_ptr<gui::Skin> m_skin;
  QPointer<QObject> m_eventReceiver;
};
}  // namespace qt::common::renderer::base

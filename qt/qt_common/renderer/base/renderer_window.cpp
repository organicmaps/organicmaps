#include "renderer_window.hpp"

#include <QCoreApplication>
#include <QExposeEvent>
#include <QResizeEvent>

namespace qt::common::renderer::base
{
RendererWindow::RendererWindow(Framework & framework, SurfaceType const surfaceType,
                               QSurfaceFormat const & surfaceFormat, QWindow * parent)
  : QWindow(parent)
  , m_framework(framework)
  , m_ratio(1.0f)
{
  setSurfaceType(surfaceType);
  setFormat(surfaceFormat);
}

RendererWindow::~RendererWindow()
{
  m_framework.SetViewportListener(nullptr);
}

dp::ApiVersion RendererWindow::GetApiVersion() const
{
  return m_framework.GetDrapeEngine()->GetApiVersion();
}

void RendererWindow::CreateDrapeEngine(dp::ApiVersion const apiVersion,
                                       ref_ptr<dp::GraphicsContextFactory> contextFactory)
{
  emit OnBeforeEngineCreation();

  Framework::DrapeCreationParams params;
  params.m_apiVersion = apiVersion;
  params.m_surfaceWidth = static_cast<int>(m_ratio * width());
  params.m_surfaceHeight = static_cast<int>(m_ratio * height());
  params.m_visualScale = m_ratio;

  m_skin.reset(new gui::Skin(gui::ResolveGuiSkinFile("default"), m_ratio));
  m_skin->Resize(params.m_surfaceWidth, params.m_surfaceHeight);
  m_skin->ForEach([&params](gui::EWidget const widget, gui::Position const & pos)
  { params.m_widgetsInitInfo[widget] = pos; });
  params.m_widgetsInitInfo[gui::WIDGET_SCALE_FPS_LABEL] = gui::Position(dp::LeftTop);

  m_framework.CreateDrapeEngine(contextFactory, std::move(params));
  m_framework.SetViewportListener([this](ScreenBase const & screen) { emit OnViewportChanged(screen); });

  emit OnAfterEngineCreation();
}

void RendererWindow::OnResize(int w, int h) const
{
  w = static_cast<int>(m_ratio * w);
  h = static_cast<int>(m_ratio * h);
  m_framework.OnSize(w, h);

  if (m_skin)
  {
    m_skin->Resize(w, h);

    gui::TWidgetsLayoutInfo layout;
    m_skin->ForEach([&layout](gui::EWidget const w, gui::Position const & pos) { layout[w] = pos.m_pixelPivot; });

    m_framework.SetWidgetLayout(std::move(layout));
  }
}

bool RendererWindow::event(QEvent * e)
{
  switch (e->type())
  {
  case QEvent::UpdateRequest: Render(); return true;
  case QEvent::Expose: exposeEvent(static_cast<QExposeEvent *>(e)); return true;
  case QEvent::Resize: resizeEvent(static_cast<QResizeEvent *>(e)); return true;
  default: break;
  }

  if (m_eventReceiver)
    return QCoreApplication::sendEvent(m_eventReceiver, e);

  return false;
}

void RendererWindow::exposeEvent(QExposeEvent * e)
{
  QWindow::exposeEvent(e);
  if (isExposed())
    requestUpdate();
}

void RendererWindow::resizeEvent(QResizeEvent * e)
{
  QWindow::resizeEvent(e);

  OnResize(e->size().width(), e->size().height());
}
}  // namespace qt::common::renderer::base

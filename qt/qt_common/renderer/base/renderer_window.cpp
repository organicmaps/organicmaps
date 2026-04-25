#include "renderer_window.hpp"

#include "qt/qt_common/renderer/base/mouse_events.hpp"

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

RendererWindow::~RendererWindow() = default;

int RendererWindow::L2D(int const px) const
{
  return static_cast<int>(px * m_ratio);
}

m2::PointD RendererWindow::GetDevicePoint(QMouseEvent const * const e) const
{
  return m2::PointD(L2D(e->position().x()), L2D(e->position().y()));
}

df::Touch RendererWindow::GetDfTouchFromQMouseEvent(QMouseEvent const * const e) const
{
  df::Touch touch;
  touch.m_id = 0;
  touch.m_location = GetDevicePoint(e);
  return touch;
}

df::TouchEvent RendererWindow::GetDfTouchEventFromQMouseEvent(QMouseEvent const * const e,
                                                              df::TouchEvent::ETouchType const type) const
{
  df::TouchEvent event;
  event.SetTouchType(type);
  event.SetFirstTouch(GetDfTouchFromQMouseEvent(e));
  if (IsCommandModifier(e))
    event.SetSecondTouch(GetSymmetrical(event.GetFirstTouch()));

  return event;
}

df::Touch RendererWindow::GetSymmetrical(df::Touch const & touch) const
{
  m2::PointD const pixelCenter = m_framework.GetVisiblePixelCenter();
  m2::PointD const symmetricalLocation = pixelCenter + pixelCenter - m2::PointD(touch.m_location);

  df::Touch result;
  result.m_id = touch.m_id + 1;
  result.m_location = symmetricalLocation;

  return result;
}

void RendererWindow::CreateDrapeEngine(dp::ApiVersion const apiVersion,
                                       ref_ptr<dp::GraphicsContextFactory> contextFactory)
{
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
  if (e->type() == QEvent::UpdateRequest)
  {
    Render();
    return true;
  }

  return QWindow::event(e);
}

void RendererWindow::mouseDoubleClickEvent(QMouseEvent * e)
{
  QWindow::mouseDoubleClickEvent(e);
  if (IsLeftButton(e))
    m_framework.Scale(Framework::SCALE_MAG_LIGHT, GetDevicePoint(e), true);
}

void RendererWindow::mousePressEvent(QMouseEvent * e)
{
  QWindow::mousePressEvent(e);
  if (IsLeftButton(e))
    m_framework.TouchEvent(GetDfTouchEventFromQMouseEvent(e, df::TouchEvent::TOUCH_DOWN));
}

void RendererWindow::mouseMoveEvent(QMouseEvent * e)
{
  QWindow::mouseMoveEvent(e);
  if (IsLeftButton(e))
    m_framework.TouchEvent(GetDfTouchEventFromQMouseEvent(e, df::TouchEvent::TOUCH_MOVE));
}

void RendererWindow::mouseReleaseEvent(QMouseEvent * e)
{
  if (IsRightButton(e))
    emit OnContextMenuRequested(e->globalPosition().toPoint());

  QWindow::mouseReleaseEvent(e);
  if (IsLeftButton(e))
    m_framework.TouchEvent(GetDfTouchEventFromQMouseEvent(e, df::TouchEvent::TOUCH_UP));
}

void RendererWindow::wheelEvent(QWheelEvent * e)
{
  QWindow::wheelEvent(e);

  QPointF const pos = e->position();

  double const factor = e->angleDelta().y() / 3.0 / 360.0;
  // https://doc-snapshots.qt.io/qt6-dev/qwheelevent.html#angleDelta, angleDelta() returns in eighths of a degree.
  m_framework.Scale(exp(factor), m2::PointD(L2D(pos.x()), L2D(pos.y())), false);
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

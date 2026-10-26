#include "qt/qt_common/map_widget_container.hpp"

#include <QtCore/QEvent>
#include <QtCore/QTimer>
#include <QtWidgets/QDockWidget>
#include <QtWidgets/QMainWindow>

namespace qt::common
{
MapWidgetContainer::MapWidgetContainer(QMainWindow & window, QWidget & map)
  : QWidget(&window)
  , m_window(window)
  , m_map(map)
{
  m_map.setParent(this);
  // Wait for the main window's initial layout before creating the map's OpenGL surface.
  m_map.hide();
  setFocusProxy(&m_map);
}

void MapWidgetContainer::UpdateGeometry()
{
  if (m_mapArea.isEmpty() || size().isEmpty())
    return;

  m_map.setGeometry(QRect(mapFrom(&m_window, m_mapArea.topLeft()), m_mapArea.size()));
  QRect const viewport(m_map.mapFrom(this, QPoint()), size());
  if (viewport != m_visibleViewport)
  {
    m_visibleViewport = viewport;
    emit VisibleViewportChanged(viewport);
  }
}

void MapWidgetContainer::ScheduleUpdate()
{
  // Sibling dock geometries can still describe the previous layout during move/resize events.
  // Reposition immediately using the last complete map area, then measure after layout finishes.
  UpdateGeometry();
  if (m_updatePending)
    return;
  m_updatePending = true;
  QTimer::singleShot(0, this, [this]
  {
    m_updatePending = false;
    if (!isVisible())
      return;

    m_mapArea = QRect(mapTo(&m_window, QPoint()), size());
    for (auto * dock : m_window.findChildren<QDockWidget *>(QString(), Qt::FindDirectChildrenOnly))
      if (dock->isVisible() && !dock->isFloating())
        m_mapArea = m_mapArea.united(dock->geometry().intersected(m_window.contentsRect()));
    UpdateGeometry();
    m_map.show();
  });
}

bool MapWidgetContainer::event(QEvent * event)
{
  bool const handled = QWidget::event(event);
  if (event->type() == QEvent::Move || event->type() == QEvent::Resize || event->type() == QEvent::Show)
    ScheduleUpdate();
  return handled;
}
}  // namespace qt::common

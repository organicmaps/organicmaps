#pragma once

#include <QtWidgets/QWidget>

class QMainWindow;

namespace qt::common
{
// Clips a map surface to the central widget while keeping it anchored underneath dock panels.
class MapWidgetContainer : public QWidget
{
  Q_OBJECT

public:
  MapWidgetContainer(QMainWindow & window, QWidget & map);

signals:
  void VisibleViewportChanged(QRect const & viewport);

protected:
  bool event(QEvent * event) override;

private:
  void UpdateGeometry();
  void ScheduleUpdate();

  QMainWindow & m_window;
  QWidget & m_map;
  QRect m_mapArea;
  QRect m_visibleViewport;
  bool m_updatePending = false;
};
}  // namespace qt::common

#pragma once

#include <QtQuickWidgets/QQuickWidget>

namespace qt
{
// QQuickWidget accepts every mouse release even if QML ignored the press: https://bugreports.qt.io/browse/QTBUG-148570
class ForwardingQuickWidget : public QQuickWidget
{
protected:
  bool event(QEvent * e) override;
  void mousePressEvent(QMouseEvent * e) override;
  void mouseDoubleClickEvent(QMouseEvent * e) override;
  void mouseReleaseEvent(QMouseEvent * e) override;

private:
  void RoutePress(QMouseEvent * e);

  Qt::MouseButtons m_ignoredButtons;
};
}  // namespace qt

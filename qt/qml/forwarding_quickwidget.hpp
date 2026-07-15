#pragma once

#include <QQuickWidget>

namespace qt
{

// A QQuickWidget that forwards mouse events to its parent.
// Workaround for https://bugreports.qt.io/browse/QTBUG-148570
class ForwardingQuickWidget : public QQuickWidget
{
protected:
  void mousePressEvent(QMouseEvent * e) override;
  void mouseReleaseEvent(QMouseEvent * e) override;

// private:
//   void forwardMouseEventToParent(QMouseEvent * e);
};

}  // namespace
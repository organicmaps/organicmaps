#include "qt/qml/forwarding_quickwidget.hpp"

#include <QtGui/QMouseEvent>
#include <QtGui/QTouchEvent>

namespace qt
{
namespace
{
// QQuickWidget accepts every TouchBegin and leaves a press ignored when QML holds only a passive grab (TapHandler).
bool HasQmlGrab(QPointerEvent const & e)
{
  for (QEventPoint const & point : e.points())
    if (e.exclusiveGrabber(point) || !e.passiveGrabbers(point).isEmpty())
      return true;
  return false;
}
}  // namespace

bool ForwardingQuickWidget::event(QEvent * e)
{
  bool const handled = QQuickWidget::event(e);
  if (e->type() != QEvent::TouchBegin || HasQmlGrab(*static_cast<QTouchEvent *>(e)))
    return handled;
  // Qt then propagates the TouchBegin to the parent, which receives the rest of the touch sequence.
  e->ignore();
  return false;
}

void ForwardingQuickWidget::mousePressEvent(QMouseEvent * e)
{
  QQuickWidget::mousePressEvent(e);
  RoutePress(e);
}

// QWidget's default mouseDoubleClickEvent turns an ignored double-click into a press, so its release must follow.
void ForwardingQuickWidget::mouseDoubleClickEvent(QMouseEvent * e)
{
  QQuickWidget::mouseDoubleClickEvent(e);
  RoutePress(e);
}

void ForwardingQuickWidget::mouseReleaseEvent(QMouseEvent * e)
{
  QQuickWidget::mouseReleaseEvent(e);
  if (m_ignoredButtons.testFlag(e->button()))
  {
    m_ignoredButtons.setFlag(e->button(), false);
    // Ignoring lets Qt propagate the release to the parent once, in parent coordinates.
    e->ignore();
  }
  else
  {
    // The release stays where its press went even when QML no longer wants it (a dismissed modal Popup's dimmer).
    e->accept();
  }
}

void ForwardingQuickWidget::RoutePress(QMouseEvent * e)
{
  // All buttons share the mouse's one point, so its grab only belongs to this press when no other button is held.
  bool const takenByQml = e->isAccepted() || (e->buttons() == e->button() && HasQmlGrab(*e));
  e->setAccepted(takenByQml);
  m_ignoredButtons.setFlag(e->button(), !takenByQml);
}
}  // namespace qt

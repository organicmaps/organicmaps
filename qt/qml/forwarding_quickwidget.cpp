#include "forwarding_quickwidget.hpp"

namespace qt
{

void ForwardingQuickWidget::mousePressEvent(QMouseEvent * e)
{
  QQuickWidget::mousePressEvent(e);
  if (!e->isAccepted() && e->button() == Qt::LeftButton)
    // forwardMouseEventToParent(e);
    QCoreApplication::sendEvent(parent(), e);
}

void ForwardingQuickWidget::mouseReleaseEvent(QMouseEvent * e)
{
  QQuickWidget::mouseReleaseEvent(e);
  if (e->button() == Qt::LeftButton)
    // forwardMouseEventToParent(e);
    QCoreApplication::sendEvent(parent(), e);
}

// void ForwardingQuickWidget::forwardMouseEventToParent(QMouseEvent * e)
// {
//   QMouseEvent forwardedEvent(e->type(), e->position(), e->position(), e->globalPosition(), e->button(), e->buttons(),
//                              e->modifiers(), e->source());
//   forwardedEvent.setTimestamp(e->timestamp());
//   QCoreApplication::sendEvent(parent(), &forwardedEvent);
//   e->setAccepted(forwardedEvent.isAccepted());
// }

}  // namespace
#include "qt/qt_common/spinner.hpp"

#include <QPainter>
#include <QTimer>

namespace
{
int constexpr kSegmentCount = 12;
int constexpr kSegmentStep = 360 / kSegmentCount;
int constexpr kTimeoutMs = 100;
int constexpr kWidgetSize = 20;
int constexpr kOuterRadius = 8;
int constexpr kSegmentWidth = 2;
int constexpr kSegmentLength = 4;
}  // namespace

Spinner::Spinner()
{
  setFixedSize(kWidgetSize, kWidgetSize);
  setEnabled(false);

  QSizePolicy policy(QSizePolicy::Fixed, QSizePolicy::Fixed);
  policy.setRetainSizeWhenHidden(true);
  setSizePolicy(policy);

  m_timer = new QTimer(this);
  connect(m_timer, &QTimer::timeout, [this]()
  {
    m_angle = (m_angle + kSegmentStep) % 360;
    update();
  });
}

void Spinner::paintEvent(QPaintEvent *)
{
  QPainter painter(this);
  painter.setRenderHint(QPainter::Antialiasing);

  auto const center = QPointF(width() / 2.0, height() / 2.0);

  for (int i = 0; i < kSegmentCount; ++i)
  {
    int const alpha = 255 * (kSegmentCount - i) / kSegmentCount;
    painter.setPen(Qt::NoPen);
    painter.setBrush(QColor(0, 0, 0, alpha));

    painter.save();
    painter.translate(center);
    painter.rotate(m_angle + i * kSegmentStep);

    painter.drawRoundedRect(-kSegmentWidth / 2, -kOuterRadius, kSegmentWidth, kSegmentLength, kSegmentWidth / 2.0,
                            kSegmentWidth / 2.0);
    painter.restore();
  }
}

void Spinner::Show()
{
  m_angle = 0;
  update();
  m_timer->start(kTimeoutMs);
  show();
}

void Spinner::Hide()
{
  m_timer->stop();
  hide();
}

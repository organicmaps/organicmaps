#include "qt/qt_common/spinner.hpp"

#include <QtCore/QString>
#include <QtCore/QTimer>
#include <QtCore/Qt>

namespace
{
int constexpr kMinSpinnerPixmap = 1;
int constexpr kMaxSpinnerPixmap = 12;
int constexpr kTimeoutMs = 100;
}  // namespace

Spinner::Spinner()
{
  for (int i = kMinSpinnerPixmap; i <= kMaxSpinnerPixmap; ++i)
  {
    auto const path = ":common/spinner" + QString::number(i) + ".png";
    m_pixmaps.emplace_back(path);
  }

  setEnabled(false);
  setAlignment(Qt::AlignCenter);

  QSizePolicy policy(QSizePolicy::Fixed, QSizePolicy::Fixed);
  policy.setRetainSizeWhenHidden(true);
  setSizePolicy(policy);

  setMinimumSize(m_pixmaps.front().size());
  setPixmap(m_pixmaps.front());

  m_timer = new QTimer(this /* parent */);
  connect(m_timer, &QTimer::timeout, [this]()
  {
    m_progress = (m_progress + 1) % m_pixmaps.size();
    setPixmap(m_pixmaps[m_progress]);
  });
}

void Spinner::Show()
{
  m_progress = 0;
  setPixmap(m_pixmaps[m_progress]);
  m_timer->start(kTimeoutMs);
  show();
}

void Spinner::Hide()
{
  m_timer->stop();
  hide();
}

#pragma once

#include <cstddef>
#include <vector>

#include <QtGui/QPixmap>
#include <QtWidgets/QLabel>

class Spinner : private QLabel
{
public:
  Spinner();

  void Show();
  void Hide();

  QLabel & AsWidget() { return static_cast<QLabel &>(*this); }

private:
  std::vector<QPixmap> m_pixmaps;
  QTimer * m_timer = nullptr;
  size_t m_progress = 0;
};

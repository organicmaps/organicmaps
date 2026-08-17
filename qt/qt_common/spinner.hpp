#pragma once

#include <QtWidgets/QWidget>

class QTimer;

class Spinner : private QWidget
{
public:
  Spinner();

  void Show();
  void Hide();

  QWidget & AsWidget() { return static_cast<QWidget &>(*this); }

protected:
  void paintEvent(QPaintEvent *) override;

private:
  QTimer * m_timer = nullptr;
  int m_angle = 0;
};

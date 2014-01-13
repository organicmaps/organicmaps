#pragma once

#include <QtWidgets/QMainWindow>

class QWidget;

class MainWindow : public QMainWindow
{
  Q_OBJECT

public:
  explicit MainWindow(QWidget *parent = 0);
  ~MainWindow();

protected:
  virtual void closeEvent(QCloseEvent * closeEvent);

private:
  QWidget * m_surface;
};

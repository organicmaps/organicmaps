#pragma once

#include <QMainWindow>

#include "glwidget.hpp"

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
  Q_OBJECT
  
public:
  explicit MainWindow(QWidget *parent = 0);
  ~MainWindow();
  
private:
  Ui::MainWindow *ui;
  GLWidget * m_widget;
};

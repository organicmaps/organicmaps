#include "mainwindow.hpp"

#include "drape_surface.hpp"

#include <QtWidgets/QWidget>

MainWindow::MainWindow(QWidget *parent)
  : QMainWindow(parent)
{
  DrapeSurface * surface = new DrapeSurface();
  setCentralWidget(QWidget::createWindowContainer(surface, this));
}

MainWindow::~MainWindow()
{
}

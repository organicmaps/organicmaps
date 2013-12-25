#include "mainwindow.hpp"

#include "drape_surface.hpp"

#include <QtWidgets/QWidget>

MainWindow::MainWindow(QWidget *parent)
  : QMainWindow(parent)
{
  resize(600, 400);
  DrapeSurface * surface = new DrapeSurface();
  setCentralWidget(QWidget::createWindowContainer(surface, this));
}

MainWindow::~MainWindow()
{
}

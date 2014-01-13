#include "mainwindow.hpp"

#include "drape_surface.hpp"

#include <QtWidgets/QWidget>

MainWindow::MainWindow(QWidget *parent)
  : QMainWindow(parent)
{
  resize(600, 400);
  DrapeSurface * surface = new DrapeSurface();
  m_surface = QWidget::createWindowContainer(surface, this);
  setCentralWidget(m_surface);
}

MainWindow::~MainWindow()
{
}

void MainWindow::closeEvent(QCloseEvent * closeEvent)
{
  delete m_surface;
}

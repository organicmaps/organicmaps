#include "mainwindow.hpp"

#include "drape_surface.hpp"

#include <QtWidgets/QWidget>

MainWindow::MainWindow(QWidget *parent)
  : QMainWindow(parent)
  , m_surface(NULL)
{
  resize(1200, 800);
  DrapeSurface * surface = new DrapeSurface();
  m_surface = QWidget::createWindowContainer(surface, this);
  setCentralWidget(m_surface);
}

MainWindow::~MainWindow()
{
  ASSERT(m_surface == NULL, ());
}

void MainWindow::closeEvent(QCloseEvent * closeEvent)
{
  delete m_surface;
  m_surface = NULL;
}

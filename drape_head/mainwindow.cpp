#include "mainwindow.hpp"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent) :
  QMainWindow(parent),
  ui(new Ui::MainWindow)
{
  ui->setupUi(this);
  m_widget = new GLWidget();
  setCentralWidget(m_widget);
}

MainWindow::~MainWindow()
{
  delete ui;
}

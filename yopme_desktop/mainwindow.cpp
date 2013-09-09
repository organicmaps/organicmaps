#include "mainwindow.hpp"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent) :
  QMainWindow(parent),
  ui(new Ui::MainWindow)
{
  ui->setupUi(this);
  m_w = new GLWidget(this);
  setCentralWidget(m_w);
}

MainWindow::~MainWindow()
{
  delete ui;
}

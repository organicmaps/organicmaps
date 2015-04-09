#include "drape_head/mainwindow.hpp"
#include <QtWidgets/QApplication>

int main(int argc, char *argv[])
{
  QApplication a(argc, argv);
  a.setQuitOnLastWindowClosed(true);

  MainWindow w;
  w.show();

  return a.exec();
}

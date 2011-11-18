#include <QtCore/QCoreApplication>

#include "mainmanager.h"

int main(int argc, char *argv[])
{
  QCoreApplication a(argc, argv);

  MainManager manager("/Users/alena/omim/omim/data/metainfo/");
  manager.ProcessCountryList("/Users/alena/omim/omim/data/polygons.lst");

  return a.exec();
}

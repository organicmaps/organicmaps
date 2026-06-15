#include "generator.hpp"

#include <iostream>
#include <stdexcept>

#include <QtCore/QDir>
#include <QtCore/QString>
#include <QtGui/QGuiApplication>

#include <gflags/gflags.h>

DEFINE_string(symbolsDir, "../../data/styles/symbols", "directory with svg symbol files");
DEFINE_int32(symbolSize, 24, "size of the rendered symbol");
DEFINE_string(outputDir, "../../data", "directory for symbols.png and symbols.xml");
DEFINE_int32(maxSize, 4096, "max width/height of output textures");

int main(int argc, char * argv[])
{
  try
  {
    gflags::ParseCommandLineFlags(&argc, &argv, true);
    QGuiApplication app(argc, argv);
    auto const outputDir = QString::fromStdString(FLAGS_outputDir);
    if (!QDir().mkpath(outputDir))
      throw std::runtime_error("Cannot create output directory " + FLAGS_outputDir);
    tools::BuildSkin(QString::fromStdString(FLAGS_symbolsDir), FLAGS_symbolSize, FLAGS_maxSize, outputDir);

    std::cout << "Done" << std::endl;
    return 0;
  }
  catch (std::exception const & e)
  {
    std::cerr << "Exception " << e.what() << std::endl;
    return -1;
  }
}

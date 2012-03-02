#include "skin_generator.hpp"

#include <QtCore/QFile>
#include <QtGui/QApplication>

#include <QtXml/QXmlSimpleReader>
#include <QtXml/QXmlInputSource>

#include "../3party/gflags/src/gflags/gflags.h"

DEFINE_string(fontFileName, "../../data/01_dejavusans.ttf", "path to TrueType font file");
DEFINE_string(symbolsFile, "../../data/results.unicode", "file with 2bytes symbols for which the skin should be generated");
DEFINE_string(symbolsDir, "../../data/styles/symbols", "directory with svg symbol files");
DEFINE_int32(symbolWidth, 24, "width of the rendered symbol");
DEFINE_int32(symbolHeight, 24, "height of the rendered symbol");
DEFINE_string(skinName, "../../data/basic", "prefix for the skin and skinImage file name");
DEFINE_string(skinSuffix, "ldpi", "suffix for skinName<suffix>.skn and symbols<suffix>.png");
DEFINE_string(searchIconsPath, "../../data/search-icons", "output path for search category icons");
DEFINE_string(searchCategories, "../../data/categories-icons.txt", "path to file that contains mapping between category and icon names");
DEFINE_int32(searchIconWidth, 42, "width of the search category icon");
DEFINE_int32(searchIconHeight, 42, "height of the search category icon");

int main(int argc, char *argv[])
{
  google::ParseCommandLineFlags(&argc, &argv, true);
  QApplication app(argc, argv);

  tools::SkinGenerator gen;

  std::vector<QSize> symbolSizes;
  symbolSizes.push_back(QSize(FLAGS_symbolWidth, FLAGS_symbolHeight));

  std::vector<std::string> suffixes;
  suffixes.push_back(FLAGS_skinSuffix);

  gen.processSearchIcons(FLAGS_symbolsDir,
                         FLAGS_searchCategories,
                         FLAGS_searchIconsPath,
                         FLAGS_searchIconWidth,
                         FLAGS_searchIconHeight);

  gen.processSymbols(FLAGS_symbolsDir, FLAGS_skinName, symbolSizes, suffixes);

  gen.renderPages();

  gen.writeToFile(FLAGS_skinName + "_" + FLAGS_skinSuffix);

  return 0;
}

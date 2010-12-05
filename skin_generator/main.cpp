#include "skin_generator.hpp"

#include <QtCore/QFile>
#include <QtGui/QApplication>

#include <QtXml/QXmlSimpleReader>
#include <QtXml/QXmlInputSource>

#include "../3party/gflags/src/gflags/gflags.h"

DEFINE_string(fontFileName, "../../data/fonts/DejaVu/DejaVuSans.ttf", "path to TrueType font file");
DEFINE_string(symbolsFile, "../../data/results.unicode", "file with 2bytes symbols for which the skin should be generated");
DEFINE_string(symbolsDir, "../../data/styles/symbols", "directory with svg symbol files");
DEFINE_int32(symbolWidth, 24, "width of the rendered symbol");
DEFINE_int32(symbolHeight, 24, "height of the rendered symbol");
DEFINE_double(symbolScale, 1, "scale factor of the symbol");
DEFINE_int32(smallGlyphSize, 12, "height of the small glyph");
DEFINE_int32(bigGlyphSize, 16, "height of the big glyph");
DEFINE_string(skinName, "../../data/basic", "prefix for the skin and skinImage file name");

int main(int argc, char *argv[])
{
  google::ParseCommandLineFlags(&argc, &argv, true);
  QApplication app(argc, argv);

  tools::SkinGenerator gen;

  std::vector<int8_t> glyphSizes;

//  glyphSizes.push_back(6);
  glyphSizes.push_back(8);
  glyphSizes.push_back(10);
  glyphSizes.push_back(12);
  glyphSizes.push_back(14);
  glyphSizes.push_back(16);
  glyphSizes.push_back(20);
  glyphSizes.push_back(24);

//  glyphSizes.push_back(FLAGS_smallGlyphSize);
//  glyphSizes.push_back(FLAGS_bigGlyphSize);

  std::vector<QSize> symbolSizes;
  symbolSizes.push_back(QSize(FLAGS_symbolWidth, FLAGS_symbolHeight));

  std::vector<double> symbolScales;
  symbolScales.push_back(FLAGS_symbolScale);

  gen.processSymbols(FLAGS_symbolsDir, FLAGS_skinName, symbolSizes, symbolScales);
  gen.processFont(FLAGS_fontFileName, FLAGS_skinName, FLAGS_symbolsFile, glyphSizes);

  gen.writeToFile(FLAGS_skinName);

  return 0;
}

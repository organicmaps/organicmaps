#pragma once

#include "../../base/base.hpp"
#include "../../std/vector.hpp"
#include "../../std/list.hpp"
#include "../../std/string.hpp"
#include "../../std/map.hpp"
#include "../../geometry/rect2d.hpp"
#include "../../coding/writer.hpp"
#include "../../geometry/packer.hpp"
#include <boost/gil/gil_all.hpp>

#include <QtGui/QPainter>
#include <QtGui/QImage>
#include <QtCore/QFileInfo>
#include <QtCore/QSize>
#include <QtSvg/QSvgRenderer>
#include <QtXml/QXmlContentHandler>
#include <QtXml/QXmlDefaultHandler>

class QImage;

namespace gil = boost::gil;

namespace tools
{
  class SkinGenerator
  {
  public:

      struct CharInfo
      {
        int m_width;
        int m_height;
        int m_xOffset;
        int m_yOffset;
        int m_xAdvance;
        gil::gray8_image_t m_image;

        m2::Packer::handle_t m_handle;
      };

      typedef map<int32_t, pair<CharInfo, CharInfo> > TChars;

      struct FontInfo
      {
        int8_t m_size;
        TChars m_chars;
      };

      typedef vector<FontInfo> TFonts;

      struct SymbolInfo
      {
        QSize m_size;
        QString m_fullFileName;
        QString m_symbolID;

        m2::Packer::handle_t m_handle;

        SymbolInfo() {}
        SymbolInfo(QSize size, QString const & fullFileName, QString const & symbolID)
          : m_size(size), m_fullFileName(fullFileName), m_symbolID(symbolID) {}
      };

      typedef vector<SymbolInfo> TSymbols;

      struct SkinPageInfo
      {
        TFonts m_fonts;
        TSymbols m_symbols;
        int m_width;
        int m_height;
        string m_fileName;
        m2::Packer m_packer;
      };

      string const getBaseFileName(string const & fileName);

  private:

      QSvgRenderer m_svgRenderer;

      int m_baseLineOffset;
      QString m_fontFileName;

      typedef vector<SkinPageInfo> TSkinPages;
      TSkinPages m_pages;

      bool m_overflowDetected;
      void markOverflow();

  public:

      SkinGenerator();
      void processFont(string const & fileName, string const & skinName, vector<int8_t> const & fontSizes, int symbolScale);
      void processSymbols(string const & symbolsDir, string const & skinName, std::vector<QSize> const & symbolSizes, std::vector<double> const & symbolScales);
      void renderPages();
      bool writeToFile(string const & skinName);
    };
} // namespace tools

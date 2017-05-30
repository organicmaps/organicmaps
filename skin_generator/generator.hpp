#pragma once

#include "geometry/rect2d.hpp"
#include "geometry/packer.hpp"

#include "coding/writer.hpp"

#include "base/base.hpp"

#include "std/vector.hpp"
#include "std/list.hpp"
#include "std/string.hpp"
#include "std/map.hpp"

#include <QtGui/QPainter>
#include <QtGui/QImage>
#include <QtCore/QFileInfo>
#include <QtCore/QSize>
#include <QtSvg/QSvgRenderer>
#include <QtXml/QXmlContentHandler>
#include <QtXml/QXmlDefaultHandler>

class QImage;

namespace tools
{
  class SkinGenerator
  {
  public:
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
        TSymbols m_symbols;
        uint32_t m_width;
        uint32_t m_height;
        string m_fileName;
        string m_dir;
        string m_suffix;
        m2::Packer m_packer;
      };

  private:

      bool m_needColorCorrection;

      QSvgRenderer m_svgRenderer;

      typedef vector<SkinPageInfo> TSkinPages;
      TSkinPages m_pages;

      bool m_overflowDetected;
      void markOverflow();

  public:

      SkinGenerator(bool needColorCorrection);
      //void processFont(string const & fileName, string const & skinName, vector<int8_t> const & fontSizes, int symbolScale);
      void processSymbols(string const & symbolsDir,
                          string const & skinName,
                          vector<QSize> const & symbolSizes,
                          vector<string> const & suffix);
      bool renderPages(uint32_t maxSize);
      bool writeToFile(string const & skinName);
      void writeToFileNewStyle(string const & skinName);
    };
} // namespace tools

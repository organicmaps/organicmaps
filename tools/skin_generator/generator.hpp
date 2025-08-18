#pragma once

#include "geometry/packer.hpp"
#include "geometry/rect2d.hpp"

#include "coding/writer.hpp"

#include "base/base.hpp"

#include <QtCore/QFileInfo>
#include <QtCore/QSize>
#include <QtGui/QImage>
#include <QtGui/QPainter>
#include <QtSvg/QSvgRenderer>

#include <cstdint>
#include <map>
#include <string>
#include <vector>

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

    m2::Packer::handle_t m_handle = {};

    SymbolInfo() {}
    SymbolInfo(QSize size, QString const & fullFileName, QString const & symbolID)
      : m_size(size)
      , m_fullFileName(fullFileName)
      , m_symbolID(symbolID)
    {}
  };

  using TSymbols = std::vector<SymbolInfo>;

  struct SkinPageInfo
  {
    TSymbols m_symbols;
    uint32_t m_width = 0;
    uint32_t m_height = 0;
    std::string m_fileName;
    std::string m_dir;
    std::string m_suffix;
    m2::Packer m_packer;
  };

  void ProcessSymbols(std::string const & symbolsDir, std::string const & skinName,
                      std::vector<QSize> const & symbolSizes, std::vector<std::string> const & suffix);
  bool RenderPages(uint32_t maxSize);
  bool WriteToFileNewStyle(std::string const & skinName);

private:
  QSvgRenderer m_svgRenderer;
  using TSkinPages = std::vector<SkinPageInfo>;
  TSkinPages m_pages;
  bool m_overflowDetected;

  void MarkOverflow();
};
}  // namespace tools

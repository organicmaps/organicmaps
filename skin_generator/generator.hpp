#pragma once

#include "geometry/rect2d.hpp"
#include "geometry/packer.hpp"

#include <QtGui/QPainter>
#include <QtCore/QFileInfo>
#include <QtCore/QSize>
#include <QtSvg/QSvgRenderer>

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
                      std::vector<QSize> const & symbolSizes,
                      std::vector<std::string> const & suffix);
  bool RenderPages(uint32_t maxSize);
  [[nodiscard]] bool WriteToFileNewStyle(std::string const & skinName) const;

private:
  QSvgRenderer m_svgRenderer;
  using TSkinPages = std::vector<SkinPageInfo>;
  TSkinPages m_pages;
  bool m_overflowDetected = false;

  void MarkOverflow();
};
}  // namespace tools

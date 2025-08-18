#include "generator.hpp"

#include "base/logging.hpp"
#include "base/math.hpp"

#include <algorithm>
#include <fstream>
#include <functional>
#include <iostream>
#include <iterator>

#include <QtCore/QDir>
#include <QtXml/QDomDocument>
#include <QtXml/QDomElement>

namespace tools
{
namespace
{

static constexpr double kLargeIconSize = 24.0;   // Size of the -l SVG icons
static constexpr double kMediumIconSize = 18.0;  // size of the -m SVG icons

struct GreaterHeight
{
  bool operator()(SkinGenerator::SymbolInfo const & left, SkinGenerator::SymbolInfo const & right) const
  {
    QString symbolIDleft = left.m_fullFileName.left(left.m_fullFileName.lastIndexOf("."));
    QString symbolIDright = right.m_fullFileName.left(right.m_fullFileName.lastIndexOf("."));
    if (left.m_size.height() == right.m_size.height())
      return symbolIDleft > symbolIDright;
    return (left.m_size.height() > right.m_size.height());
  }
};

struct MaxDimensions
{
  uint32_t & m_width;
  uint32_t & m_height;

  MaxDimensions(uint32_t & width, uint32_t & height) : m_width(width), m_height(height)
  {
    m_width = 0;
    m_height = 0;
  }

  void operator()(SkinGenerator::SymbolInfo const & info)
  {
    m_width = std::max(std::max(m_width, m_height), static_cast<uint32_t>(info.m_size.width()));
    m_height = std::max(std::max(m_width, m_height), static_cast<uint32_t>(info.m_size.height()));
  }
};

uint32_t NextPowerOf2(uint32_t n)
{
  n = n - 1;
  n |= (n >> 1);
  n |= (n >> 2);
  n |= (n >> 4);
  n |= (n >> 8);
  n |= (n >> 16);

  return n + 1;
}
}  // namespace

void SkinGenerator::ProcessSymbols(std::string const & svgDataDir, std::string const & skinName,
                                   std::vector<QSize> const & symbolSizes, std::vector<std::string> const & suffixes)
{
  for (size_t j = 0; j < symbolSizes.size(); ++j)
  {
    QDir dir(QString(svgDataDir.c_str()));
    QStringList fileNames = dir.entryList(QDir::Files);

    QDir pngDir(dir.absolutePath() + "/png");
    fileNames += pngDir.entryList(QDir::Files);

    // Separate page for symbols.
    m_pages.emplace_back(SkinPageInfo());
    SkinPageInfo & page = m_pages.back();

    page.m_dir = skinName.substr(0, skinName.find_last_of("/") + 1);
    page.m_suffix = suffixes[j];
    page.m_fileName = page.m_dir + "symbols" + page.m_suffix;

    for (int i = 0; i < fileNames.size(); ++i)
    {
      QString const & fileName = fileNames.at(i);
      QString symbolID = fileName.left(fileName.lastIndexOf("."));
      if (fileName.endsWith(".svg"))
      {
        QString fullFileName = QString(dir.absolutePath()) + "/" + fileName;
        if (m_svgRenderer.load(fullFileName))
        {
          QSize svgSize = m_svgRenderer.defaultSize();  // Size of the SVG file

          // Capping svg symbol to kLargeIconSize maximum, keeping aspect ratio
          /*if (svgSize.width() > kLargeIconSize)
          {
            auto const h = static_cast<float>(svgSize.height()) * kLargeIconSize / svgSize.width();
            svgSize.setHeight(static_cast<int>(h));
            svgSize.setWidth(kLargeIconSize);
          }

          if (svgSize.height() > kLargeIconSize)
          {
            auto const w = static_cast<float>(svgSize.width()) * kLargeIconSize / svgSize.height();
            svgSize.setWidth(static_cast<int>(w));
            svgSize.setHeight(kLargeIconSize);
          }*/

          // Scale symbol to required size
          QSize size = svgSize * (symbolSizes[j].width() / kMediumIconSize);

          page.m_symbols.emplace_back(size + QSize(4, 4), fullFileName, symbolID);
        }
      }
      else if (fileName.toLower().endsWith(".png"))
      {
        QString fullFileName = QString(pngDir.absolutePath()) + "/" + fileName;
        QPixmap pix(fullFileName);
        QSize s = pix.size();
        page.m_symbols.emplace_back(s + QSize(4, 4), fullFileName, symbolID);
      }
    }
  }
}

bool SkinGenerator::RenderPages(uint32_t maxSize)
{
  for (auto & page : m_pages)
  {
    std::sort(page.m_symbols.begin(), page.m_symbols.end(), GreaterHeight());

    MaxDimensions dim(page.m_width, page.m_height);
    for_each(page.m_symbols.begin(), page.m_symbols.end(), dim);

    page.m_width = NextPowerOf2(page.m_width);
    page.m_height = NextPowerOf2(page.m_height);

    // Packing until we find a suitable rect.
    while (true)
    {
      page.m_packer = m2::Packer(page.m_width, page.m_height);
      page.m_packer.addOverflowFn(std::bind(&SkinGenerator::MarkOverflow, this), 10);

      m_overflowDetected = false;

      for (auto & s : page.m_symbols)
      {
        s.m_handle =
            page.m_packer.pack(static_cast<uint32_t>(s.m_size.width()), static_cast<uint32_t>(s.m_size.height()));
        if (m_overflowDetected)
          break;
      }

      if (m_overflowDetected)
      {
        // Enlarge packing area and try again.
        if (page.m_width == page.m_height)
          page.m_width *= 2;
        else
          page.m_height *= 2;

        if (page.m_width > maxSize)
        {
          page.m_width = maxSize;
          page.m_height *= 2;
          if (page.m_height > maxSize)
            return false;
        }
        continue;
      }
      break;
    }
    LOG(LINFO, ("Texture size =", page.m_width, "x", page.m_height));

    std::vector<uchar> imgData(page.m_width * page.m_height * 4, 0);
    QImage img(imgData.data(), page.m_width, page.m_height, QImage::Format_ARGB32);
    QPainter painter(&img);
    painter.setClipping(true);

    for (auto const & s : page.m_symbols)
    {
      m2::RectU dstRect = page.m_packer.find(s.m_handle).second;
      QRect dstRectQt(dstRect.minX(), dstRect.minY(), dstRect.SizeX(), dstRect.SizeY());

      painter.fillRect(dstRectQt, QColor(0, 0, 0, 0));

      painter.setClipRect(dstRect.minX() + 2, dstRect.minY() + 2, dstRect.SizeX() - 4, dstRect.SizeY() - 4);
      QRect renderRect(dstRect.minX() + 2, dstRect.minY() + 2, dstRect.SizeX() - 4, dstRect.SizeY() - 4);

      QString fullLowerCaseName = s.m_fullFileName.toLower();
      if (fullLowerCaseName.endsWith(".svg"))
      {
        m_svgRenderer.load(s.m_fullFileName);
        m_svgRenderer.render(&painter, renderRect);
      }
      else if (fullLowerCaseName.endsWith(".png"))
      {
        QPixmap pix(s.m_fullFileName);
        painter.drawPixmap(renderRect, pix);
      }
    }

    std::string s = page.m_fileName + ".png";
    LOG(LINFO, ("saving skin image into: ", s));
    img.save(s.c_str());
  }

  return true;
}

void SkinGenerator::MarkOverflow()
{
  m_overflowDetected = true;
}

bool SkinGenerator::WriteToFileNewStyle(std::string const & skinName)
{
  QDomDocument doc = QDomDocument("skin");
  QDomElement rootElem = doc.createElement("root");
  doc.appendChild(rootElem);

  for (auto const & p : m_pages)
  {
    QDomElement fileNode = doc.createElement("file");
    fileNode.setAttribute("width", p.m_width);
    fileNode.setAttribute("height", p.m_height);
    rootElem.appendChild(fileNode);

    for (auto const & s : p.m_symbols)
    {
      m2::RectU r = p.m_packer.find(s.m_handle).second;
      QDomElement symbol = doc.createElement("symbol");
      symbol.setAttribute("minX", r.minX());
      symbol.setAttribute("minY", r.minY());
      symbol.setAttribute("maxX", r.maxX());
      symbol.setAttribute("maxY", r.maxY());
      symbol.setAttribute("name", s.m_symbolID.toLower());
      fileNode.appendChild(symbol);
    }
  }
  QFile file(QString(skinName.c_str()));
  if (!file.open(QIODevice::ReadWrite | QIODevice::Truncate))
    return false;
  QTextStream ts(&file);
  ts.setEncoding(QStringConverter::Utf8);
  ts << doc.toString();
  return true;
}
}  // namespace tools

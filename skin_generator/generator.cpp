#include "generator.hpp"

#include "base/logging.hpp"
#include "base/math.hpp"

#include "std/algorithm.hpp"
#include "std/iterator.hpp"
#include "std/fstream.hpp"
#include "std/iostream.hpp"
#include "std/bind.hpp"

#include <QtXml/QDomElement>
#include <QtXml/QDomDocument>
#include <QtCore/QDir>

/// @todo(greshilov): delete this hack for next boost version (>1.65.0)
#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wc++11-narrowing"
#endif
#include <boost/gil/gil_all.hpp>
#ifdef __clang__
#pragma clang diagnostic pop
#endif

namespace gil = boost::gil;

namespace tools
{
  SkinGenerator::SkinGenerator(bool needColorCorrection)
    : m_needColorCorrection(needColorCorrection)
  {}

  struct GreaterHeight
  {
    bool operator() (SkinGenerator::SymbolInfo const & left,
                     SkinGenerator::SymbolInfo const & right) const
    {
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
      m_width = max(max(m_width, m_height), static_cast<uint32_t>(info.m_size.width()));
      m_height = max(max(m_width, m_height), static_cast<uint32_t>(info.m_size.height()));
    }
  };

  int NextPowerOf2(int n)
  {
    n = n - 1;
    n |= (n >> 1);
    n |= (n >> 2);
    n |= (n >> 4);
    n |= (n >> 8);
    n |= (n >> 16);

    return n + 1;
  }

  void DoPatchSize(QString const & name, string const & skinName, QSize & size)
  {
    if (name.startsWith("placemark-") || name.startsWith("current-position") || name.startsWith("api_pin"))
    {
      if (skinName.rfind("-mdpi") != string::npos)
        size = QSize(24, 24);
      else if (skinName.rfind("-hdpi") != string::npos)
        size = QSize(36, 36);
      else if (skinName.rfind("-xhdpi") != string::npos)
        size = QSize(48, 48);
      else if (skinName.rfind("-xxhdpi") != string::npos)
        size = QSize(72, 72);
    }
  }

  void SkinGenerator::processSymbols(string const & svgDataDir,
                                     string const & skinName,
                                     vector<QSize> const & symbolSizes,
                                     vector<string> const & suffixes)
  {
    for (size_t j = 0; j < symbolSizes.size(); ++j)
    {
      QDir dir(QString(svgDataDir.c_str()));
      QStringList fileNames = dir.entryList(QDir::Files);

      QDir pngDir = dir.absolutePath() + "/" + "png";
      fileNames += pngDir.entryList(QDir::Files);

      /// separate page for symbols
      m_pages.push_back(SkinPageInfo());
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
            QSize defaultSize = m_svgRenderer.defaultSize();

            QSize symbolSize = symbolSizes[j];
            DoPatchSize(fileName, skinName, symbolSize);

            QSize size = defaultSize * (symbolSize.width() / 24.0);

            /// fitting symbol into symbolSize, saving aspect ratio

            if (size.width() > symbolSize.width())
            {
              size.setHeight((float)size.height() * symbolSize.width() / (float)size.width());
              size.setWidth(symbolSize.width());
            }

            if (size.height() > symbolSize.height())
            {
              size.setWidth((float)size.width() * symbolSize.height() / (float)size.height());
              size.setHeight(symbolSize.height());
            }

            page.m_symbols.push_back(SymbolInfo(size + QSize(4, 4), fullFileName, symbolID));
          }
        }
        else if (fileName.toLower().endsWith(".png"))
        {
          QString fullFileName = QString(pngDir.absolutePath()) + "/" + fileName;
          QPixmap pix(fullFileName);
          QSize s = pix.size();
          page.m_symbols.push_back(SymbolInfo(s + QSize(4, 4), fullFileName, symbolID));
        }
      }
    }
  }

  namespace
  {
    void correctColors(gil::bgra8_image_t & image)
    {
      gil::bgra8_view_t view = gil::view(image);
      for (gil::bgra8_view_t::y_coord_t y = 0; y < view.height(); ++y)
      {
        for (gil::bgra8_view_t::x_coord_t x = 0; x < view.width(); ++x)
        {
          gil::bgra8_pixel_t pixel = view(x, y);
          unsigned char color =
              my::clamp(0.07 * pixel[0] + 0.5 * pixel[1] + 0.22 * pixel[2], 0.0, 255.0);

          view(x, y)[0] = color;
          view(x, y)[1] = color;
          view(x, y)[2] = color;
        }
      }
    }
  }

  bool SkinGenerator::renderPages(uint32_t maxSize)
  {
    for (TSkinPages::iterator pageIt = m_pages.begin(); pageIt != m_pages.end(); ++pageIt)
    {
      SkinPageInfo & page = *pageIt;
      sort(page.m_symbols.begin(), page.m_symbols.end(), GreaterHeight());

      MaxDimensions dim(page.m_width, page.m_height);
      for_each(page.m_symbols.begin(), page.m_symbols.end(), dim);

      page.m_width = NextPowerOf2(page.m_width);
      page.m_height = NextPowerOf2(page.m_height);

      /// packing until we find a suitable rect
      while (true)
      {
        page.m_packer = m2::Packer(page.m_width, page.m_height);
        page.m_packer.addOverflowFn(bind(&SkinGenerator::markOverflow, this), 10);

        m_overflowDetected = false;

        for (TSymbols::iterator it = page.m_symbols.begin(); it != page.m_symbols.end(); ++it)
        {
          it->m_handle = page.m_packer.pack(it->m_size.width(), it->m_size.height());
          if (m_overflowDetected)
            break;
        }

        if (m_overflowDetected)
        {
          /// enlarge packing area and try again
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

      gil::bgra8_image_t gilImage(page.m_width, page.m_height);
      gil::fill_pixels(gil::view(gilImage), gil::rgba8_pixel_t(0, 0, 0, 0));
      QImage img((uchar*)&gil::view(gilImage)(0, 0), page.m_width, page.m_height, QImage::Format_ARGB32);
      QPainter painter(&img);
      painter.setClipping(true);

      for (TSymbols::const_iterator it = page.m_symbols.begin(); it != page.m_symbols.end(); ++it)
      {
        m2::RectU dstRect = page.m_packer.find(it->m_handle).second;
        QRect dstRectQt(dstRect.minX(), dstRect.minY(), dstRect.SizeX(), dstRect.SizeY());

        painter.fillRect(dstRectQt, QColor(0, 0, 0, 0));

        painter.setClipRect(dstRect.minX() + 2, dstRect.minY() + 2, dstRect.SizeX() - 4, dstRect.SizeY() - 4);
        QRect renderRect(dstRect.minX() + 2, dstRect.minY() + 2, dstRect.SizeX() - 4, dstRect.SizeY() - 4);

        QString fullLowerCaseName = it->m_fullFileName.toLower();
        if (fullLowerCaseName.endsWith(".svg"))
        {
          m_svgRenderer.load(it->m_fullFileName);
          m_svgRenderer.render(&painter, renderRect);
        }
        else if (fullLowerCaseName.endsWith(".png"))
        {
          QPixmap pix(it->m_fullFileName);
          painter.drawPixmap(renderRect, pix);
        }
      }

      string s = page.m_fileName + ".png";
      LOG(LINFO, ("saving skin image into: ", s));
      if (m_needColorCorrection)
        correctColors(gilImage);
      img.save(s.c_str());
    }

    return true;
  }

  void SkinGenerator::markOverflow()
  {
    m_overflowDetected = true;
  }

  void SkinGenerator::writeToFileNewStyle(const string & skinName)
  {
    QDomDocument doc = QDomDocument("skin");
    QDomElement rootElem = doc.createElement("root");
    doc.appendChild(rootElem);

    for (vector<SkinPageInfo>::const_iterator pageIt = m_pages.begin(); pageIt != m_pages.end(); ++pageIt)
    {
      QDomElement fileNode = doc.createElement("file");
      fileNode.setAttribute("width", pageIt->m_width);
      fileNode.setAttribute("height", pageIt->m_height);
      rootElem.appendChild(fileNode);

      for (vector<SymbolInfo>::const_iterator symbolIt = pageIt->m_symbols.begin();
           symbolIt != pageIt->m_symbols.end(); ++symbolIt)
      {
        m2::RectU r = pageIt->m_packer.find(symbolIt->m_handle).second;
        QDomElement symbol = doc.createElement("symbol");
        symbol.setAttribute("minX", r.minX());
        symbol.setAttribute("minY", r.minY());
        symbol.setAttribute("maxX", r.maxX());
        symbol.setAttribute("maxY", r.maxY());
        symbol.setAttribute("name", symbolIt->m_symbolID.toLower());
        fileNode.appendChild(symbol);
      }
    }
    string extName = ".sdf";
    QFile file(QString((skinName + extName).c_str()));
    if (!file.open(QIODevice::ReadWrite | QIODevice::Truncate))
      throw std::exception();
    QTextStream ts(&file);
    ts.setCodec("UTF-8");
    ts << doc.toString();
  }
}

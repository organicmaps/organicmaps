#include "skin_generator.hpp"

#include "../coding/lodepng_io.hpp"

#include "../base/logging.hpp"
#include "../base/math.hpp"

#include "../std/algorithm.hpp"
#include "../std/iterator.hpp"
#include "../std/fstream.hpp"
#include "../std/iostream.hpp"
#include "../std/bind.hpp"

#include <QtXml/QDomElement>
#include <QtXml/QDomDocument>
#include <QtCore/QDir>

namespace tools
{
  SkinGenerator::SkinGenerator(bool needColorCorrection)
    : m_baseLineOffset(0), m_needColorCorrection(needColorCorrection)
  {}

  string const SkinGenerator::getBaseFileName(string const & fileName)
  {
    int startPos = fileName.find_last_of("/");
    int endPos = fileName.find_last_of(".");
    if (endPos != string::npos)
      endPos = endPos - startPos - 1;

    string s = fileName.substr(fileName.find_last_of("/") + 1, endPos);
    for (size_t i = 0; i < s.size(); ++i)
      s[i] = tolower(s[i]);
    return s;
  }

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
    int & m_width;
    int & m_height;

    MaxDimensions(int & width, int & height)
      : m_width(width), m_height(height)
    {
      m_width = 0;
      m_height = 0;
    }

    void operator()(SkinGenerator::SymbolInfo const & info)
    {
      m_width = max(m_width, info.m_size.width());
      m_height = max(m_height, info.m_size.height());
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

  void SkinGenerator::processSearchIcons(string const & symbolsDir,
                                         string const & searchCategories,
                                         string const & searchIconsPath,
                                         int searchIconWidth,
                                         int searchIconHeight)
  {
    ifstream fin(searchCategories.c_str());
    QDir().mkpath(QString(searchIconsPath.c_str()));

    while (true)
    {
      string category;
      string icon;
      fin >> category;
      fin >> icon;
      if (!fin)
        break;

      QString fullFileName((symbolsDir + "/" + icon + ".svg").c_str());

      if (m_svgRenderer.load(fullFileName))
      {
        QRect viewBox = m_svgRenderer.viewBox();
        QSize defaultSize = m_svgRenderer.defaultSize();

        QSize size = defaultSize * (searchIconWidth / 24.0);

        /// fitting symbol into symbolSize, saving aspect ratio

        if (size.width() > searchIconWidth)
        {
          size.setHeight((float)size.height() * searchIconWidth / (float)size.width());
          size.setWidth(searchIconWidth);
        }

        if (size.height() > searchIconHeight)
        {
          size.setWidth((float)size.width() * searchIconHeight / (float)size.height());
          size.setHeight(searchIconHeight);
        }

        renderIcon(symbolsDir + "/" + icon + ".svg",
                   searchIconsPath + "/" + category + ".png",
                   size);

        renderIcon(symbolsDir + "/" + icon + ".svg",
                   searchIconsPath + "/" + category + "@2x.png",
                   size * 2);
      }
      else
        LOG(LERROR, ("hasn't found icon", icon, "for category", category));
    };
  }

  void SkinGenerator::renderIcon(string const & svgFile,
                                 string const & pngFile,
                                 QSize const & size)
  {
    if (m_svgRenderer.load(QString(svgFile.c_str())))
    {
      gil::bgra8_image_t gilImage(size.width(), size.height());
      gil::fill_pixels(gil::view(gilImage), gil::rgba8_pixel_t(0, 0, 0, 0));
      QImage img((uchar*)&gil::view(gilImage)(0, 0), size.width(), size.height(), QImage::Format_ARGB32);
      QPainter painter(&img);

      m_svgRenderer.render(&painter, QRect(0, 0, size.width(), size.height()));
      img.save(pngFile.c_str());
    }
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

      /// separate page for symbols
      m_pages.push_back(SkinPageInfo());
      SkinPageInfo & page = m_pages.back();

      page.m_dir = skinName.substr(0, skinName.find_last_of("/") + 1);
      page.m_suffix = suffixes[j];
      page.m_fileName = page.m_dir + "symbols" + page.m_suffix;

      for (size_t i = 0; i < fileNames.size(); ++i)
      {
        QString const & fileName = fileNames.at(i);
        if (fileName.endsWith(".svg"))
        {
          QString fullFileName = QString(svgDataDir.c_str()) + "/" + fileName;
          QString symbolID = fileName.left(fileName.lastIndexOf("."));
          if (m_svgRenderer.load(fullFileName))
          {
            QRect viewBox = m_svgRenderer.viewBox();
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
          unsigned char color = my::clamp(0.07 * pixel[0] + 0.5 * pixel[1] + 0.22  * pixel[2], 0, 255);

          view(x, y)[0] = color;
          view(x, y)[1] = color;
          view(x, y)[2] = color;
        }
      }
    }
  }

  void SkinGenerator::renderPages()
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

        m_svgRenderer.load(it->m_fullFileName);
        QRect renderRect(dstRect.minX() + 2, dstRect.minY() + 2, dstRect.SizeX() - 4, dstRect.SizeY() - 4);
        m_svgRenderer.render(&painter, renderRect);
      }

      string s = page.m_fileName + ".png";
      LOG(LINFO, ("saving skin image into: ", s));
      if (m_needColorCorrection)
        correctColors(gilImage);
      img.save(s.c_str());
    }
  }

  void SkinGenerator::markOverflow()
  {
    m_overflowDetected = true;
  }

  bool SkinGenerator::writeToFile(std::string const & skinName)
  {
    /// Creating Data file
    QDomDocument doc = QDomDocument("skin");
    QDomElement skinElem = doc.createElement("skin");
    doc.appendChild(skinElem);

    for (vector<SkinPageInfo>::const_iterator it = m_pages.begin(); it != m_pages.end(); ++it)
    {
      SkinPageInfo const & page = *it;

      QDomElement pageElem = doc.createElement("page");
      skinElem.appendChild(pageElem);
      pageElem.setAttribute("width", page.m_width);
      pageElem.setAttribute("height", page.m_height);
      pageElem.setAttribute("file", (page.m_fileName.substr(page.m_fileName.find_last_of("/") + 1) + ".png").c_str());

      int minDynamicID = 0;
      int maxFontResourceID = 0;

      for (TFonts::const_iterator fontIt = page.m_fonts.begin(); fontIt != page.m_fonts.end(); ++fontIt)
      {
        QDomElement fontInfo = doc.createElement("fontInfo");
        fontInfo.setAttribute("size", fontIt->m_size);

        for (TChars::const_iterator it = fontIt->m_chars.begin(); it != fontIt->m_chars.end(); ++it)
        {
          QDomElement charStyle = doc.createElement("charStyle");

          charStyle.setAttribute("id", it->first);

          QDomElement glyphInfo = doc.createElement("glyphInfo");
          charStyle.appendChild(glyphInfo);

          QDomElement resourceStyle = doc.createElement("resourceStyle");

          m2::RectU texRect = page.m_packer.find(it->second.first.m_handle).second;

          resourceStyle.setAttribute("x", texRect.minX());
          resourceStyle.setAttribute("y", texRect.minY());
          resourceStyle.setAttribute("width", texRect.SizeX());
          resourceStyle.setAttribute("height", texRect.SizeY());

          glyphInfo.appendChild(resourceStyle);

          glyphInfo.setAttribute("xAdvance", it->second.first.m_xAdvance);
          glyphInfo.setAttribute("xOffset", it->second.first.m_xOffset);
          glyphInfo.setAttribute("yOffset", it->second.first.m_yOffset);

          QDomElement glyphMaskInfo = doc.createElement("glyphMaskInfo");
          resourceStyle = doc.createElement("resourceStyle");

          texRect = page.m_packer.find(it->second.second.m_handle).second;

          resourceStyle.setAttribute("x", texRect.minX());
          resourceStyle.setAttribute("y", texRect.minY());
          resourceStyle.setAttribute("width", texRect.SizeX());
          resourceStyle.setAttribute("height", texRect.SizeY());

          glyphMaskInfo.appendChild(resourceStyle);
          glyphMaskInfo.setAttribute("xAdvance", it->second.second.m_xAdvance);
          glyphMaskInfo.setAttribute("xOffset", it->second.second.m_xOffset);
          glyphMaskInfo.setAttribute("yOffset", it->second.second.m_yOffset);

          charStyle.appendChild(glyphMaskInfo);

          fontInfo.appendChild(charStyle);

          maxFontResourceID = max(it->first, maxFontResourceID);
        }

        pageElem.appendChild(fontInfo);
      }

      minDynamicID += maxFontResourceID + 1;
      int maxImageResourceID = 0;

      for (vector<SymbolInfo>::const_iterator it = page.m_symbols.begin(); it != page.m_symbols.end(); ++it)
      {
        QDomElement symbolStyle = doc.createElement("symbolStyle");

        QDomElement resourceStyle = doc.createElement("resourceStyle");

        m2::RectU r = page.m_packer.find(it->m_handle).second;

        resourceStyle.setAttribute("x", r.minX());
        resourceStyle.setAttribute("y", r.minY());
        resourceStyle.setAttribute("width", r.SizeX());
        resourceStyle.setAttribute("height", r.SizeY());

        symbolStyle.appendChild(resourceStyle);
        symbolStyle.setAttribute("id", minDynamicID + it->m_handle);
        symbolStyle.setAttribute("name", it->m_symbolID.toLower());

        maxImageResourceID = max(maxImageResourceID, (int)it->m_handle);

        pageElem.appendChild(symbolStyle);
      }

      minDynamicID += maxImageResourceID + 1;
    }

    QFile::remove(QString((skinName + ".skn").c_str()));

    if (QFile::exists((skinName + ".skn").c_str()))
      throw std::exception();

    QFile file(QString((skinName + ".skn").c_str()));

    LOG(LINFO, ("writing skin into ", skinName + ".skn"));

    if (!file.open(QIODevice::ReadWrite))
      throw std::exception();
    QTextStream ts(&file);
    ts.setCodec("UTF-8");
    ts << doc.toString();

    return true;
  }
}

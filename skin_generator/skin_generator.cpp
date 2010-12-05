#include "skin_generator.hpp"
#include <QtXml/QDomElement>
#include <QtXml/QDomDocument>
#include <QtCore/QDir>
#include "../std/bind.hpp"
#include "../coding/lodepng_io.hpp"
#include <boost/gil/gil_all.hpp>
#include "../std/algorithm.hpp"
#include "../std/iterator.hpp"
#include "../std/fstream.hpp"
#include "../std/iostream.hpp"

#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_GLYPH_H


namespace gil = boost::gil;

namespace tools
{
  SkinGenerator::SkinGenerator()
    : m_baseLineOffset(0)
  {}

  void SkinGenerator::processFont(string const & fileName, string const & skinName, string const & symFreqFile, vector<int8_t> const & fontSizes)
  {
    FILE * file = fopen(symFreqFile.c_str(), "rb");
    if (!file)
      throw std::exception();
    std::vector<unsigned short> ucs2Symbols;

    while (true)
    {
      unsigned short id;
      int readBytes = fread(&id, 1, sizeof(unsigned short), file);
      if (readBytes < 2)
        break;
      ucs2Symbols.push_back(id);
    }

    fclose(file);

    FT_Library lib;
    FT_Init_FreeType(&lib);

    FT_Face face;
    FT_New_Face(lib, fileName.c_str(), 0, &face);

    FT_Glyph_Metrics glyphMetrics;

    for (size_t i = 0; i < fontSizes.size(); ++i)
    {
      m_pages.push_back(SkinPageInfo());
      SkinPageInfo & page = m_pages.back();

      page.m_fonts.push_back(FontInfo());
      FontInfo & fontInfo = page.m_fonts.back();

      fontInfo.m_size = fontSizes[i];

      FT_Set_Pixel_Sizes(face, 0, fontSizes[i]);
      for (size_t j = 0; j < ucs2Symbols.size(); ++j)
      {
        unsigned short symbol = ucs2Symbols[j];

        int symbolIdx = FT_Get_Char_Index(face, symbol);

        if (symbolIdx == 0)
          continue;

        FT_Load_Glyph(face, symbolIdx, FT_LOAD_DEFAULT);
        glyphMetrics = face->glyph->metrics;

        CharInfo charInfo;
        charInfo.m_width = int(glyphMetrics.width >> 6);
        charInfo.m_height = int(glyphMetrics.height >> 6);
        charInfo.m_xOffset = int(glyphMetrics.horiBearingX >> 6);
        charInfo.m_yOffset = int(glyphMetrics.horiBearingY >> 6) - charInfo.m_height;
        charInfo.m_xAdvance = int(glyphMetrics.horiAdvance >> 6);

        FT_GlyphSlot glyphSlot = face->glyph;
        if ((charInfo.m_width != 0) && (charInfo.m_height != 0))
        {
          FT_Render_Glyph(glyphSlot, FT_RENDER_MODE_NORMAL);

          typedef gil::gray8_pixel_t pixel_t;

          gil::gray8c_view_t grayview = gil::interleaved_view(
              charInfo.m_width,
              charInfo.m_height,
              (pixel_t*)glyphSlot->bitmap.buffer,
              sizeof(unsigned char) * glyphSlot->bitmap.width);

          charInfo.m_image.recreate(charInfo.m_width, charInfo.m_height);
          gil::copy_pixels(grayview, gil::view(charInfo.m_image));
        }

        fontInfo.m_chars[symbol] = charInfo;
      }

      std::stringstream out;
      out << getBaseFileName(fileName) + "_" << (int)fontSizes[i];

      page.m_fileName = out.str().c_str();

      /// repacking symbols as tight as possible
      page.m_width = 32;
      page.m_height = 32;

      while (true)
      {
        m_overflowDetected = false;

        page.m_packer = m2::Packer(page.m_width, page.m_height);
        page.m_packer.addOverflowFn(bind(&SkinGenerator::markOverflow, this), 10);

        for (TChars::iterator it = fontInfo.m_chars.begin(); it != fontInfo.m_chars.end(); ++it)
        {
          it->second.m_handle = page.m_packer.pack(it->second.m_width + 4, it->second.m_height + 4);
          if (m_overflowDetected)
            break;
        }

        if (m_overflowDetected)
        {
          if (page.m_width == page.m_height)
            page.m_width *= 2;
          else
            page.m_height *= 2;
          continue;
        }
        else
          break;
      }

      gil::bgra8_image_t skinImage(page.m_width, page.m_height);
      gil::fill_pixels(gil::view(skinImage), gil::rgba8_pixel_t(0, 0, 0, 0));

      for (TChars::const_iterator it = fontInfo.m_chars.begin(); it != fontInfo.m_chars.end(); ++it)
      {
        m2::RectU dstRect(page.m_packer.find(it->second.m_handle).second);

        gil::rgba8_pixel_t color(0, 0, 0, 0);

        gil::bgra8_view_t dstView = gil::subimage_view(gil::view(skinImage), dstRect.minX(), dstRect.minY(), dstRect.SizeX(), dstRect.SizeY());
        gil::fill_pixels(dstView, color);

        dstView = gil::subimage_view(gil::view(skinImage),
                                     dstRect.minX() + 2,
                                     dstRect.minY() + 2,
                                     dstRect.SizeX() - 4,
                                     dstRect.SizeY() - 4);

        gil::gray8c_view_t srcView = gil::const_view(it->second.m_image);

        for (size_t x = 0; x < dstRect.SizeX() - 4; ++x)
          for (size_t y = 0; y < dstRect.SizeY() - 4; ++y)
          {
            color[3] = srcView(x, y);
            dstView(x, y) = color;
          }
      }

      gil::lodepng_write_view(
          skinName.substr(0, skinName.find_last_of("/") + 1) + page.m_fileName + ".png",
          gil::const_view(skinImage));
    }

    FT_Done_Face(face);
    FT_Done_FreeType(lib);
  }

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

  struct LessHeight
  {
    bool operator()(SkinGenerator::SymbolInfo const & left, SkinGenerator::SymbolInfo const & right)
    {
      return left.m_size.height() < right.m_size.height();
    }
  };

  void SkinGenerator::processSymbols(string const & svgDataDir, string const & skinName, std::vector<QSize> const & symbolSizes, std::vector<double> const & symbolScales)
  {
    for (int i = 0; i < symbolSizes.size(); ++i)
    {
      QDir dir(QString(svgDataDir.c_str()));
      QStringList fileNames = dir.entryList(QDir::Files);

      /// separate page for symbols
      m_pages.push_back(SkinPageInfo());
      SkinPageInfo & page = m_pages.back();

      double symbolScale = symbolScales[i];
      QSize symbolSize = symbolSizes[i];

      for (int i = 0; i < fileNames.size(); ++i)
      {
        if (fileNames.at(i).endsWith(".svg"))
        {
          QString fullFileName = QString(svgDataDir.c_str()) + "/" + fileNames.at(i);
          QString symbolID = fileNames.at(i).left(fileNames.at(i).lastIndexOf("."));
          if (m_svgRenderer.load(fullFileName))
          {
            QRect viewBox = m_svgRenderer.viewBox();
            QSize defaultSize = m_svgRenderer.defaultSize();

            QSize size = defaultSize * symbolScale;

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

      /// Trying to repack symbols as tight as possible
      page.m_width = 64;
      page.m_height = 64;

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
          {
            /// enlarge packing area and try again
            if (page.m_width == page.m_height)
              page.m_width *= 2;
            else
              page.m_height *= 2;
            break;
          }
        }

        if (m_overflowDetected)
          continue;

        break;
      }

      /// rendering packed symbols

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
        m_svgRenderer.render(&painter, QRect(dstRect.minX() + 2, dstRect.minY() + 2, dstRect.SizeX() - 4, dstRect.SizeY() - 4));
      }

      page.m_fileName = skinName.substr(0, skinName.find_last_of("/") + 1) + "symbols_" + QString("%1").arg(symbolSize.width()).toLocal8Bit().constData();

      img.save((page.m_fileName + ".png").c_str());
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
          QDomElement resourceStyle = doc.createElement("resourceStyle");

          m2::RectU const & texRect = page.m_packer.find(it->second.m_handle).second;

          resourceStyle.setAttribute("x", texRect.minX());
          resourceStyle.setAttribute("y", texRect.minY());
          resourceStyle.setAttribute("width", texRect.SizeX());
          resourceStyle.setAttribute("height", texRect.SizeY());

          charStyle.appendChild(resourceStyle);

          charStyle.setAttribute("xAdvance", it->second.m_xAdvance);
          charStyle.setAttribute("xOffset", it->second.m_xOffset);
          charStyle.setAttribute("yOffset", it->second.m_yOffset);
          charStyle.setAttribute("id", it->first);

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

    if (!file.open(QIODevice::ReadWrite))
      throw std::exception();
    QTextStream ts(&file);
    ts.setCodec("UTF-8");
    ts << doc.toString();

    return true;
  }
}

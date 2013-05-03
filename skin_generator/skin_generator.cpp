#include "skin_generator.hpp"

#include "../coding/lodepng_io.hpp"

#include "../base/logging.hpp"

#include "../std/algorithm.hpp"
#include "../std/iterator.hpp"
#include "../std/fstream.hpp"
#include "../std/iostream.hpp"
#include "../std/bind.hpp"

#include <QtXml/QDomElement>
#include <QtXml/QDomDocument>
#include <QtCore/QDir>


/*
#include <ft2build.h>

#include FT_FREETYPE_H
#include FT_STROKER_H
#include FT_GLYPH_H

#undef __FTERRORS_H__
#define FT_ERRORDEF( e, v, s )  { e, s },
#define FT_ERROR_START_LIST     {
#define FT_ERROR_END_LIST       { 0, 0 } };

const struct
{
  int          err_code;
  const char*  err_msg;
} ft_errors[] =

#include FT_ERRORS_H

void CheckError(FT_Error error)
{
  if (error != 0)
  {
    int i = 0;
    while (ft_errors[i].err_code != 0)
    {
      if (ft_errors[i].err_code == error)
      {
        LOG(LERROR, (ft_errors[i].err_msg));
        break;
      }
      ++i;
    }
  }
}

#define FTCHECK(x) do {FT_Error e = (x); CheckError(e);} while (false)

namespace gil = boost::gil;
*/

namespace tools
{
  SkinGenerator::SkinGenerator()
    : m_baseLineOffset(0)
  {}

  /*
  void SkinGenerator::processFont(string const & fileName, string const & skinName, vector<int8_t> const & fontSizes, int symbolScale)
  {
    string symbols(" 0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ;:'\"/?.,`~!@#$%^&*()-_+=<>");

    FT_Library lib;
    FT_Error error;
    FTCHECK(FT_Init_FreeType(&lib));

    FT_Face face;
    FTCHECK(FT_New_Face(lib, fileName.c_str(), 0, &face));

    FT_Stroker stroker;
    FTCHECK(FT_Stroker_New(lib, &stroker));
    size_t outlineWidth = 2;
    FT_Stroker_Set(stroker, outlineWidth * 64, FT_STROKER_LINECAP_ROUND, FT_STROKER_LINEJOIN_ROUND, 0);

    FT_Glyph_Metrics normalGlyphMetrics;
    FT_Glyph_Metrics strokedGlyphMetrics;

    for (size_t i = 0; i < fontSizes.size(); ++i)
    {
//      m_pages.push_back(SkinPageInfo());
      SkinPageInfo & page = m_pages.back();

      page.m_fonts.push_back(FontInfo());
      FontInfo & fontInfo = page.m_fonts.back();

      fontInfo.m_size = fontSizes[i];

      FTCHECK(FT_Set_Pixel_Sizes(face, 0, fontSizes[i]));
      for (size_t j = 0; j < symbols.size(); ++j)
      {
        unsigned short symbol = (unsigned short)symbols[j];

        int symbolIdx = FT_Get_Char_Index(face, symbol);

        if (symbolIdx == 0)
          continue;

        FTCHECK(FT_Load_Glyph(face, symbolIdx, FT_LOAD_DEFAULT));

        normalGlyphMetrics = face->glyph->metrics;

        CharInfo normalCharInfo;

        normalCharInfo.m_width = int(normalGlyphMetrics.width >> 6);
        normalCharInfo.m_height = int(normalGlyphMetrics.height >> 6);
        normalCharInfo.m_xOffset = int(normalGlyphMetrics.horiBearingX >> 6);
        normalCharInfo.m_yOffset = int(normalGlyphMetrics.horiBearingY >> 6) - normalCharInfo.m_height;
        normalCharInfo.m_xAdvance = int(normalGlyphMetrics.horiAdvance >> 6);

        CharInfo strokedCharInfo = normalCharInfo;

        if ((normalCharInfo.m_width != 0) && (normalCharInfo.m_height != 0))
        {
          FT_GlyphSlot glyphSlot = face->glyph;

          FT_Glyph strokedGlyph;
          FTCHECK(FT_Get_Glyph(face->glyph, &strokedGlyph));

          FTCHECK(FT_Glyph_Stroke(&strokedGlyph, stroker, 1));
          FTCHECK(FT_Glyph_To_Bitmap(&strokedGlyph, FT_RENDER_MODE_NORMAL, 0, 0));

          FT_BitmapGlyph strokedBitmapGlyph = (FT_BitmapGlyph)strokedGlyph;

          strokedCharInfo.m_width = strokedBitmapGlyph->bitmap.width;
          strokedCharInfo.m_height = strokedBitmapGlyph->bitmap.rows;
          strokedCharInfo.m_xOffset = strokedBitmapGlyph->left;
          strokedCharInfo.m_yOffset = int(strokedBitmapGlyph->top) - strokedCharInfo.m_height;
          strokedCharInfo.m_xAdvance = int(strokedBitmapGlyph->root.advance.x >> 16);

          typedef gil::gray8_pixel_t pixel_t;

          gil::gray8c_view_t grayview = gil::interleaved_view(
              strokedCharInfo.m_width,
              strokedCharInfo.m_height,
              (pixel_t*)strokedBitmapGlyph->bitmap.buffer,
              strokedBitmapGlyph->bitmap.pitch);

          strokedCharInfo.m_image.recreate(strokedCharInfo.m_width,
                                           strokedCharInfo.m_height);


          gil::copy_pixels(grayview, gil::view(strokedCharInfo.m_image));

          FT_Done_Glyph(strokedGlyph);

          FTCHECK(FT_Render_Glyph(glyphSlot, FT_RENDER_MODE_NORMAL));

          FT_Glyph normalGlyph;
          FTCHECK(FT_Get_Glyph(face->glyph, &normalGlyph));

          FT_BitmapGlyph normalBitmapGlyph = (FT_BitmapGlyph)normalGlyph;

          grayview = gil::interleaved_view(
              normalCharInfo.m_width,
              normalCharInfo.m_height,
              (pixel_t*)normalBitmapGlyph->bitmap.buffer,
              sizeof(unsigned char) * normalBitmapGlyph->bitmap.width);

          normalCharInfo.m_image.recreate(normalCharInfo.m_width, normalCharInfo.m_height);

          gil::copy_pixels(grayview, gil::view(normalCharInfo.m_image));

          FT_Done_Glyph(normalGlyph);
        }

        fontInfo.m_chars[symbol] = make_pair(normalCharInfo, strokedCharInfo);
      }

      //std::stringstream out;
      //out << getBaseFileName(fileName) + "_" << (int)fontSizes[i];

      //page.m_fileName = out.str().c_str();

      //gil::bgra8_image_t skinImage(page.m_width, page.m_height);
      //gil::fill_pixels(gil::view(skinImage), gil::rgba8_pixel_t(0, 0, 0, 0));

      //gil::lodepng_write_view(
      //    skinName.substr(0, skinName.find_last_of("/") + 1) + page.m_fileName + ".png",
      //    gil::const_view(skinImage));
    }

    FT_Done_Face(face);
    FT_Done_FreeType(lib);
  }
  */

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

  void SkinGenerator::processSymbols(string const & svgDataDir,
                                     string const & skinName,
                                     vector<QSize> const & symbolSizes,
                                     vector<string> const & suffixes)
  {
    for (int i = 0; i < symbolSizes.size(); ++i)
    {
      QDir dir(QString(svgDataDir.c_str()));
      QStringList fileNames = dir.entryList(QDir::Files);

      /// separate page for symbols
      m_pages.push_back(SkinPageInfo());
      SkinPageInfo & page = m_pages.back();

      QSize symbolSize = symbolSizes[i];

      page.m_dir = skinName.substr(0, skinName.find_last_of("/") + 1);
      page.m_suffix = suffixes[i];
      page.m_fileName = page.m_dir + "symbols" + page.m_suffix;

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
  void SkinGenerator::renderPages()
  {
    for (TSkinPages::iterator pageIt = m_pages.begin(); pageIt != m_pages.end(); ++pageIt)
    {
      SkinPageInfo & page = *pageIt;
      /// Trying to repack all elements as tight as possible
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

        for (TFonts::iterator fontIt = page.m_fonts.begin(); fontIt != page.m_fonts.end(); ++fontIt)
        {
          for (TChars::iterator charIt = fontIt->m_chars.begin(); charIt != fontIt->m_chars.end(); ++charIt)
          {
            charIt->second.first.m_handle = page.m_packer.pack(
                charIt->second.first.m_width + 4,
                charIt->second.first.m_height + 4);
            if (m_overflowDetected)
             break;

            charIt->second.second.m_handle = page.m_packer.pack(
                charIt->second.second.m_width + 4,
                charIt->second.second.m_height + 4);

            if (m_overflowDetected)
              break;
          }
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
        m_svgRenderer.render(&painter, QRect(dstRect.minX() + 2, dstRect.minY() + 2, dstRect.SizeX() - 4, dstRect.SizeY() - 4));

        size_t w = dstRect.SizeX() - 4;
        size_t h = dstRect.SizeY() - 4;

        gil::bgra8_image_t symbolImagePng(w, h);
        gil::fill_pixels(gil::view(symbolImagePng), gil::rgba8_pixel_t(0, 0, 0, 0));
        QImage img((uchar*)&gil::view(symbolImagePng)(0, 0), w, h, QImage::Format_ARGB32);
        QPainter painter(&img);
        painter.setClipping(true);

        m_svgRenderer.load(it->m_fullFileName);
        m_svgRenderer.render(&painter, QRect(0, 0, w, h));
        string dir(page.m_dir + "icons/");
        QDir().mkpath(QString(dir.c_str()));
        string s(dir + it->m_symbolID.toLocal8Bit().constData() + ".png");
        img.save(s.c_str());
      }

      /// Rendering packed fonts
      for (TFonts::const_iterator fontIt = page.m_fonts.begin(); fontIt != page.m_fonts.end(); ++fontIt)
      {
        for (TChars::const_iterator charIt = fontIt->m_chars.begin(); charIt != fontIt->m_chars.end(); ++charIt)
        {
          /// Packing normal char
          m2::RectU dstRect(page.m_packer.find(charIt->second.first.m_handle).second);

          gil::rgba8_pixel_t color(0, 0, 0, 0);

          gil::bgra8_view_t dstView = gil::subimage_view(gil::view(gilImage), dstRect.minX(), dstRect.minY(), dstRect.SizeX(), dstRect.SizeY());
          gil::fill_pixels(dstView, color);

          dstView = gil::subimage_view(gil::view(gilImage),
                                       dstRect.minX() + 2,
                                       dstRect.minY() + 2,
                                       dstRect.SizeX() - 4,
                                       dstRect.SizeY() - 4);

          gil::gray8c_view_t srcView = gil::const_view(charIt->second.first.m_image);

          for (size_t x = 0; x < dstRect.SizeX() - 4; ++x)
            for (size_t y = 0; y < dstRect.SizeY() - 4; ++y)
            {
              color[3] = srcView(x, y);
              dstView(x, y) = color;
            }

          /// packing stroked version

          dstRect = m2::RectU(page.m_packer.find(charIt->second.second.m_handle).second);

          color = gil::rgba8_pixel_t(255, 255, 255, 0);

          dstView = gil::subimage_view(gil::view(gilImage), dstRect.minX(), dstRect.minY(), dstRect.SizeX(), dstRect.SizeY());
          gil::fill_pixels(dstView, color);

          dstView = gil::subimage_view(gil::view(gilImage),
                                       dstRect.minX() + 2,
                                       dstRect.minY() + 2,
                                       dstRect.SizeX() - 4,
                                       dstRect.SizeY() - 4);

          srcView = gil::const_view(charIt->second.second.m_image);

          for (size_t x = 0; x < dstRect.SizeX() - 4; ++x)
            for (size_t y = 0; y < dstRect.SizeY() - 4; ++y)
            {
              color[3] = srcView(x, y);
              dstView(x, y) = color;
            }

        }
      }

      string s = page.m_fileName + ".png";
      LOG(LINFO, ("saving skin image into: ", s));
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

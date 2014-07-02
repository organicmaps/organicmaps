#include "engine.hpp"

#define STB_TRUETYPE_IMPLEMENTATION
#include "stb_tt.hpp"
#include "BinPacker.hpp"
#include "df_map.hpp"

#include <QFile>
#include <QTextStream>

#include "../base/macros.hpp"
#include "../base/logging.hpp"

#include "../std/cmath.hpp"
#include "../std/vector.hpp"
#include "../std/map.hpp"

#include "../std/function.hpp"
#include "../std/bind.hpp"

#include <boost/gil/typedefs.hpp>
#include <boost/gil/algorithm.hpp>

using boost::gil::gray8c_view_t;
using boost::gil::gray8_view_t;
using boost::gil::gray8c_pixel_t;
using boost::gil::gray8_pixel_t;
using boost::gil::interleaved_view;
using boost::gil::subimage_view;
using boost::gil::copy_pixels;

typedef function<void (void)> simple_fun_t;
typedef function<void (int)> int_fun_t;

namespace
{
  //static int GlyphScaler = 16;
  static int EtalonTextureSize = 1024;
  struct ZeroPoint
  {
    int32_t  m_x, m_y;
  };

  class AtlasCompositor
  {
  public:
    AtlasCompositor(int pageCount)
    {
      InitMetrics(pageCount);
    }

    int GetWidth() const
    {
      return m_width;
    }

    int GetHeight() const
    {
      return m_height;
    }

    ZeroPoint GetZeroPoint(int pageNumber) const
    {
      ZeroPoint zPoint;
      zPoint.m_x = pageNumber % GetWidth();
      zPoint.m_y = pageNumber / GetWidth();
      return zPoint;
    }

  private:
    struct MetricTemplate
    {
      MetricTemplate(int pageCount, int width, int height)
        : m_pageCount(pageCount)
        , m_width(width)
        , m_height(height)
      {
      }

      int32_t  m_pageCount;
      int32_t  m_width;
      int32_t  m_height;
    };

    void InitMetrics(int32_t  pageCount)
    {
      static MetricTemplate templates[] =
      {
        MetricTemplate(1, 1, 1),
        MetricTemplate(2, 2, 1),
        MetricTemplate(3, 2, 2),
        MetricTemplate(4, 2, 2),
        MetricTemplate(5, 3, 2),
        MetricTemplate(6, 3, 2),
        MetricTemplate(7, 3, 3),
        MetricTemplate(8, 3, 3),
        MetricTemplate(9, 3, 3),
        MetricTemplate(10, 4, 3),
        MetricTemplate(11, 4, 3),
        MetricTemplate(12, 4, 3),
        MetricTemplate(13, 4, 4),
        MetricTemplate(14, 4, 4),
        MetricTemplate(15, 4, 4),
        MetricTemplate(16, 4, 4)
      };

      for (unsigned long i = 0; i < ARRAY_SIZE(templates); ++ i)
      {
        if (templates[i].m_pageCount == pageCount)
        {
          m_width = templates[i].m_width;
          m_height = templates[i].m_height;
        }
      }
    }

  private:
    int32_t  m_width;
    int32_t  m_height;
  };

  class MyThread : public QThread
  {
  public:

    struct GlyphInfo
    {
      int32_t m_unicodePoint;
      int32_t m_glyphIndex;
      int32_t m_x, m_y;
      int32_t m_width, m_height;
      float m_xoff;
      float m_yoff;
      float m_advance;
      vector<uint8_t> m_img;
    };

    MyThread(QList<FontRange> const & fonts, int fontSize,
             int_fun_t const & startFn,
             int_fun_t const & updateFn,
             simple_fun_t const & endFn)
      : m_start(startFn)
      , m_update(updateFn)
      , m_end(endFn)
      , m_fontSize(fontSize)
    {
      m_summaryGlyphCount = 0;
      foreach(FontRange r, fonts)
      {
        if (r.m_validFont)
        {
          m_summaryGlyphCount += ((r.m_endRange - r.m_startRange) + 1);
          m_fonts[r.m_fontPath].push_back(qMakePair(r.m_startRange, r.m_endRange));
        }
      }
    }

    void run()
    {
      m_start(m_summaryGlyphCount);
      m_summaryGlyphCount = 0;
      QList<GlyphInfo> infos;
      QList<GlyphInfo> emptyInfos;
      for (map_iter_t font = m_fonts.begin(); font != m_fonts.end(); ++font)
      {
        QFile f(font.key());
        if (f.open(QIODevice::ReadOnly) == false)
          throw 1;

        stbtt_fontinfo fontInfo;
        {
          vector<uint8_t> fontBuffer(f.size(), 0);
          f.read((char *)&fontBuffer[0], fontBuffer.size());
          stbtt_InitFont(&fontInfo, &fontBuffer[0], 0);
        }

        float scale = stbtt_ScaleForPixelHeight(&fontInfo, /*GlyphScaler * */m_fontSize);
        for (range_iter_t range = font.value().begin(); range != font.value().end(); ++range)
        {
          for (int unicodeCode = range->first; unicodeCode <= range->second; ++unicodeCode)
          {
            m_update(m_summaryGlyphCount);
            m_summaryGlyphCount++;
            if (isInterruptionRequested())
              return;

            int width = 0, height = 0, xoff = 0, yoff = 0;
            int glyphCode = stbtt_FindGlyphIndex(&fontInfo, unicodeCode);
            if (glyphCode == 0)
              continue;
            unsigned char * image = NULL;
            if (!stbtt_IsGlyphEmpty(&fontInfo, glyphCode))
              image = stbtt_GetGlyphBitmap(&fontInfo, scale, scale, glyphCode, &width, &height, &xoff, &yoff);

            int advance = 0, leftBear = 0;
            stbtt_GetGlyphHMetrics(&fontInfo, glyphCode, &advance, &leftBear);

            GlyphInfo info;
            info.m_unicodePoint = unicodeCode;
            info.m_glyphIndex = glyphCode;
            info.m_width = width;
            info.m_height = height;
            info.m_xoff = xoff /*/ (float)GlyphScaler*/;
            info.m_yoff = yoff /*/ (float)GlyphScaler*/;
            info.m_advance = advance * (scale /*/ (float) GlyphScaler*/);
            if (info.m_width == 0 || info.m_height == 0)
            {
              emptyInfos.push_back(info);
            }
            else
            {
              processGlyph(image, width, height, info.m_img, info.m_width, info.m_height);
              infos.push_back(info);
              stbtt_FreeBitmap(image, NULL);
            }
          }
        }
      }

      vector<int> rects;
      rects.reserve(2 * infos.size());
      foreach(GlyphInfo info, infos)
      {
        if (!info.m_img.empty())
        {
          rects.push_back(info.m_width + 1);
          rects.push_back(info.m_height + 1);
        }
      }

      rects.push_back(4);
      rects.push_back(4);

      vector< vector<int> > outRects;

      BinPacker bp;
      bp.Pack(rects, outRects, EtalonTextureSize, false);

      AtlasCompositor compositor(outRects.size());

      m_w = EtalonTextureSize * compositor.GetWidth();
      m_h = EtalonTextureSize * compositor.GetHeight();

      m_image.resize(m_w * m_h);
      memset(&m_image[0], 0, m_image.size() * sizeof(uint8_t));

      gray8_view_t resultView = interleaved_view(m_w, m_h,
                                                 (gray8_pixel_t *)&m_image[0],
                                                 m_w);

      bool firstEmpty = true;
      for (size_t k = 0; k < outRects.size(); ++k)
      {
        vector<int> & outValues = outRects[k];
        for (size_t i = 0; i < outValues.size(); i += 4)
        {
          int id = outValues[i];
          if (id != infos.size())
          {
            GlyphInfo & info = infos[id];
            ZeroPoint zPoint = compositor.GetZeroPoint(k);
            info.m_x = EtalonTextureSize * zPoint.m_x + outValues[i + 1];
            info.m_y = EtalonTextureSize * zPoint.m_y + outValues[i + 2];

            int packX = info.m_x;
            int packY = info.m_y;

            gray8_view_t subResultView = subimage_view(resultView,
                                                       packX + 1, packY + 1,
                                                       info.m_width, info.m_height);
            gray8c_view_t symbolView = interleaved_view(info.m_width, info.m_height,
                                                        (gray8c_pixel_t *)&info.m_img[0],
                                                        info.m_width);
            copy_pixels(symbolView, subResultView);
          }
          else
          {
            Q_ASSERT(firstEmpty);
            for (int j = 0; j < emptyInfos.size(); ++j)
            {
              emptyInfos[j].m_x = outValues[i + 1];
              emptyInfos[j].m_y = outValues[i + 2];
            }

            firstEmpty = false;
          }
        }
      }

      m_infos.clear();
      m_infos.append(infos);
      m_infos.append(emptyInfos);
    }

    vector<uint8_t> const & GetImage(int & w, int & h) const
    {
      w = m_w;
      h = m_h;
      return m_image;
    }

    QList<GlyphInfo> const & GetInfos() const { return m_infos; }

  private:
    static void processGlyph(unsigned char * glyphImage, int32_t width, int32_t height,
                             vector<uint8_t> & image, int32_t & newW, int32_t & newH)
    {
      static uint32_t border = 2;
      int32_t const sWidth = width + 2 * border;
      int32_t const sHeight = height + 2 * border;

      image.resize(sWidth * sHeight);
      memset(&image[0], 0, image.size() * sizeof(uint8_t));
      gray8_view_t bufView = interleaved_view(sWidth, sHeight,
                                              (gray8_pixel_t *)&image[0],
                                              sWidth);
      gray8_view_t subView = subimage_view(bufView, border, border, width, height);
      gray8c_view_t srcView = interleaved_view(width, height,
                                               (gray8c_pixel_t *)glyphImage,
                                               width);

      for (gray8c_view_t::y_coord_t y = 0; y < subView.height(); ++y)
      {
        for (gray8c_view_t::x_coord_t x = 0; x < subView.width(); ++x)
        {
          if (srcView(x, y) > 40)
            subView(x, y) = 255;
          else
            subView(x, y) = 0;
        }
      }

      DFMap forwardMap(image, sWidth, sHeight, 255, 0);
      DFMap inverseMap(image, sWidth, sHeight, 0, 255);
      forwardMap.Minus(inverseMap);
      forwardMap.Normalize();
      forwardMap.GenerateImage(image, newW, newH);
    }

  private:
    int_fun_t m_start;
    int_fun_t m_update;
    simple_fun_t m_end;

  private:
    int m_summaryGlyphCount;
    typedef QPair<int, int> range_t;
    typedef QList<range_t> ranges_t;
    typedef ranges_t::const_iterator range_iter_t;
    typedef QMap<QString, ranges_t> map_t;
    typedef map_t::const_iterator map_iter_t;
    map_t m_fonts;
    vector<uint8_t> m_image;
    int m_w, m_h;
    int m_fontSize;
    QList<GlyphInfo> m_infos;
  };
}

Engine::Engine()
  : m_workThread(NULL)
{
}

void Engine::SetFonts(QList<FontRange> const & fonts, int fontSize)
{
  if (m_workThread != NULL)
  {
    disconnect(m_workThread, SIGNAL(finished()), this, SLOT(WorkThreadFinished()));
    connect(m_workThread, SIGNAL(finished()), m_workThread, SLOT(deleteLater()));
    if (!m_workThread->isFinished())
      m_workThread->requestInterruption();
    else
      delete m_workThread;
  }

  m_workThread = new MyThread(fonts, fontSize,
                              bind(&Engine::StartEngine, this, _1),
                              bind(&Engine::UpdateProgress, this, _1),
                              bind(&Engine::EndEngine, this));
  connect(m_workThread, SIGNAL(finished()), this, SLOT(WorkThreadFinished()));
  m_workThread->start();
}

void Engine::SetExportPath(const QString & dirName)
{
  m_dirName = dirName;
}

bool Engine::IsReadyToExport() const
{
  return !m_dirName.isEmpty() && m_workThread;
}

void Engine::RunExport()
{
  Q_ASSERT(IsReadyToExport() == true);
  if (!m_workThread->isFinished())
    m_workThread->wait();

  GetImage().save(m_dirName.trimmed() + "/font.png", "png");

  MyThread * thread = static_cast<MyThread *>(m_workThread);
  QList<MyThread::GlyphInfo> const & infos = thread->GetInfos();

  QFile file(m_dirName.trimmed() + "/font.txt");
  if (!file.open(QIODevice::WriteOnly))
    throw -1;

  QTextStream stream(&file);
  for (int i = 0; i < infos.size(); ++i)
  {
    MyThread::GlyphInfo const & info = infos[i];
    stream << info.m_unicodePoint << "\t"
           << info.m_x << "\t"
           << info.m_y << "\t"
           << info.m_width << "\t"
           << info.m_height << "\t"
           << info.m_xoff << "\t"
           << info.m_yoff << "\t"
           << info.m_advance << "\n";
  }
}

void Engine::WorkThreadFinished()
{
  QThread * t = qobject_cast<QThread *>(sender());
  if (t == NULL)
    return;

  emit UpdatePreview(GetImage(t));
}

QImage Engine::GetImage(QThread * sender) const
{
  MyThread * thread = static_cast<MyThread *>(sender == NULL ? m_workThread : sender);
  int w, h;
  vector<uint8_t> const & imgData = thread->GetImage(w, h);

  QImage image = QImage(&imgData[0], w, h, QImage::Format_Indexed8);
  image.setColorCount(256);
  for (int i = 0; i < 256; ++i)
    image.setColor(i, qRgb(i, i, i));

  return image;
}

void Engine::EmitStartEngine(int maxValue)
{
  emit StartEngine(maxValue);
}

void Engine::EmitUpdateProgress(int currentValue)
{
  emit UpdateProgress(currentValue);
}

void Engine::EmitEndEngine()
{
  emit EndEngine();
}

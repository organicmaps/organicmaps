#include "engine.hpp"

#define STB_TRUETYPE_IMPLEMENTATION
#include "stb_tt.hpp"
#include "BinPacker.hpp"
#include "df_map.hpp"

#include <QMap>
#include <QList>
#include <QFile>
#include <QTextStream>

#include "../base/macros.hpp"

#include "../std/cmath.hpp"
#include "../std/vector.hpp"
#include "../std/map.hpp"

#include "../std/function.hpp"
#include "../std/bind.hpp"

typedef function<void (void)> simple_fun_t;
typedef function<void (int)> int_fun_t;

namespace
{
  //static int GlyphScaler = 16;
  static int EtalonTextureSize = 1024;
  struct ZeroPoint
  {
    int m_x, m_y;
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

      int m_pageCount;
      int m_width;
      int m_height;
    };

    void InitMetrics(int pageCount)
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
    int m_width;
    int m_height;
  };

  class MyThread : public QThread
  {
  public:

    struct GlyphInfo
    {
      int m_unicodePoint;
      int m_glyphIndex;
      int m_x, m_y;
      int m_width, m_height;
      float m_xoff;
      float m_yoff;
      float m_advance;
      unsigned char * m_img;
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
            info.m_img = NULL;
            if (info.m_width == 0 || info.m_height == 0)
            {
              Q_ASSERT(info.m_img == NULL);
              emptyInfos.push_back(info);
            }
            else
            {
              info.m_img = processGlyph(image, width, height, info.m_width, info.m_height);
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
        if (info.m_img != NULL)
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

      int width = EtalonTextureSize * compositor.GetWidth();
      int height = EtalonTextureSize * compositor.GetHeight();

      vector<uint8_t> resultImg(width * height, 0);
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

            for (int y = 0; y < info.m_height; ++y)
            {
              for (int x = 0; x < info.m_width; ++x)
              {
                int dstX = packX + x + 1;
                int dstY = packY + y + 1;
                resultImg[dstX + dstY * width] = info.m_img[x + y * info.m_width];
              }
            }
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

      foreach (GlyphInfo info, infos)
        delete[] info.m_img;

      m_image = QImage(&resultImg[0], width, height, QImage::Format_Indexed8);
      m_image.setColorCount(256);
      for (int i = 0; i < 256; ++i)
        m_image.setColor(i, qRgb(i, i, i));

      m_infos.clear();
      m_infos.append(infos);
      m_infos.append(emptyInfos);
    }

    QImage GetImage() const
    {
      return m_image;
    }

    QList<GlyphInfo> const & GetInfos() const { return m_infos; }

  private:
    static unsigned char * processGlyph(unsigned char * glyphImage, int width, int height,
                                        int & newW, int & newH)
    {
      static int border = 2;
      int sWidth = width + 4 * border;
      int sHeight = height + 4 * border;

      vector<unsigned char> buf(sWidth * sHeight, 0);
      for (int y = 0; y < height; ++y)
        for (int x = 0; x < width; ++x)
        {
          if (glyphImage[x + y * width] < 127)
            buf[2 * border + x + (2 * border + y) * sWidth] = 0;
          else
            buf[2 * border + x + (2 * border + y) * sWidth] = 255;
        }

      DFMap forwardMap(buf, sWidth, sHeight, 255, 0);
      DFMap inverseMap(buf, sWidth, sHeight, 0, 255);
      forwardMap.Minus(inverseMap);
      forwardMap.Normalize();
      return forwardMap.GenerateImage(newW, newH);
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
    QImage m_image;
    int m_fontSize;
    QList<GlyphInfo> m_infos;
  };
}

Engine::Engine()
  : m_workThread(NULL)
{
}

void Engine::SetFonts(const QList<FontRange> & fonts, int fontSize)
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

  MyThread * thread = static_cast<MyThread *>(m_workThread);
  thread->GetImage().save(m_dirName.trimmed() + "/font.png", "png");
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

  MyThread * thread = static_cast<MyThread *>(t);
  emit UpdatePreview(thread->GetImage());
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

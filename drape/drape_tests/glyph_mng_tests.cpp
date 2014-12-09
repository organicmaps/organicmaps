#include "../../testing/testing.hpp"


#include <QtWidgets/QApplication>
#include <QtWidgets/QWidget>
#include <QtGui/QPainter>
#include <QtCore/QTimer>

#include "../glyph_manager.hpp"
#include "../../platform/platform.hpp"
#include "../../base/scope_guard.hpp"

#include "../../std/cstring.hpp"
#include "../../std/function.hpp"
#include "../../std/bind.hpp"

namespace
{
  class TestMainLoop : public QObject
  {
  public:
    typedef function<void (QPaintDevice *)> TRednerFn;
    TestMainLoop(TRednerFn const & fn) : m_renderFn(fn) {}

    void exec(char const * testName)
    {
      char * buf = (char *)malloc(strlen(testName) + 1);
      MY_SCOPE_GUARD(argvFreeFun, [&buf](){ free(buf); })
      strcpy(buf, testName);

      int argc = 1;
      QApplication app(argc, &buf);
      QTimer::singleShot(3000, &app, SLOT(quit()));

      QWidget w;
      w.setWindowTitle(testName);
      w.show();
      w.installEventFilter(this);

      app.exec();
    }

  protected:
    bool eventFilter(QObject * obj, QEvent * event)
    {
      if (event->type() == QEvent::Paint)
      {
        m_renderFn(qobject_cast<QWidget *>(obj));
        return true;
      }

      return false;
    }

  private:
    TRednerFn m_renderFn;
  };

  class GlyphRenderer
  {
  public:
    GlyphRenderer()
    {
      dp::GlyphManager::Params args;
      args.m_uniBlocks = "unicode_blocks.txt";
      args.m_whitelist = "fonts_whitelist.txt";
      args.m_blacklist = "fonts_blacklist.txt";
      GetPlatform().GetFontNames(args.m_fonts);

      m_mng = new dp::GlyphManager(args);
    }

    ~GlyphRenderer()
    {
      delete m_mng;
    }

    void RenderGlyphs(QPaintDevice * device)
    {
      vector<dp::GlyphManager::Glyph> glyphs;
      m_mng->GetGlyphs({0x58, 0x79, 0x439}, glyphs);

      QPainter painter(device);
      painter.fillRect(QRectF(0.0, 0.0, device->width(), device->height()), Qt::white);

      QPoint pen(100, 100);
      for (dp::GlyphManager::Glyph & g : glyphs)
      {
        if (!g.m_image.m_data)
          continue;

        uint8_t * d = SharedBufferManager::GetRawPointer(g.m_image.m_data);
        int pitch = 32 * (((g.m_image.m_width - 1) / 32) + 1);
        int byteCount = pitch * g.m_image.m_height;
        unsigned char * buf = (unsigned char *)malloc(byteCount);
        memset(buf, 0, byteCount);
        for (int i = 0; i < g.m_image.m_height; ++i)
          memcpy(buf + pitch * i, d + g.m_image.m_width * i, g.m_image.m_width);

        QImage img = QImage(buf,
                            pitch,
                            g.m_image.m_height,
                            QImage::Format_Indexed8);

        img.setColorCount(0xFF);
        for (int i = 0; i < 256; ++i)
          img.setColor(i, qRgb(255 - i, 255 - i, 255 - i));
        QPoint currentPen = pen;
        currentPen.rx() += g.m_metrics.m_xOffset;
        currentPen.ry() -= g.m_metrics.m_yOffset;
        painter.drawImage(currentPen, img, QRect(0, 0, g.m_image.m_width, g.m_image.m_height));
        pen.rx() += g.m_metrics.m_xAdvance;
        pen.ry() += g.m_metrics.m_yAdvance;

        free(buf);
        g.m_image.Destroy();
      }
    }

  private:
    dp::GlyphManager * m_mng;
  };
}

UNIT_TEST(GlyphLoadingTest)
{
  GlyphRenderer renderer;
  TestMainLoop loop(bind(&GlyphRenderer::RenderGlyphs, &renderer, _1));
  loop.exec("GlyphLoadingTest");
}

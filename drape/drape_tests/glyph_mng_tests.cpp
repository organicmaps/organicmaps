#include "testing/testing.hpp"

#include "drape/drape_tests/img.hpp"

#include <QtGui/QPainter>

#include "qt_tstfrm/test_main_loop.hpp"

#include "drape/glyph_manager.hpp"
#include "platform/platform.hpp"

#include "std/cstring.hpp"
#include "std/function.hpp"
#include "std/bind.hpp"

namespace
{
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
      auto generateGlyph = [this, &glyphs](strings::UniChar c)
      {
        dp::GlyphManager::Glyph g = m_mng->GetGlyph(c);
        glyphs.push_back(m_mng->GenerateGlyph(g));
        g.m_image.Destroy();
      };

      generateGlyph(0xC0);
      generateGlyph(0x79);
      generateGlyph(0x122);

      QPainter painter(device);
      painter.fillRect(QRectF(0.0, 0.0, device->width(), device->height()), Qt::white);

      QPoint pen(100, 100);
      for (dp::GlyphManager::Glyph & g : glyphs)
      {
        if (!g.m_image.m_data)
          continue;

        uint8_t * d = SharedBufferManager::GetRawPointer(g.m_image.m_data);

        QPoint currentPen = pen;
        currentPen.rx() += g.m_metrics.m_xOffset;
        currentPen.ry() -= g.m_metrics.m_yOffset;
        painter.drawImage(currentPen, CreateImage(g.m_image.m_width, g.m_image.m_height, d), QRect(0, 0, g.m_image.m_width, g.m_image.m_height));
        pen.rx() += g.m_metrics.m_xAdvance;
        pen.ry() += g.m_metrics.m_yAdvance;

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
  RunTestLoop("GlyphLoadingTest", bind(&GlyphRenderer::RenderGlyphs, &renderer, _1));
}

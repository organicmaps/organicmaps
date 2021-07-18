#include "testing/testing.hpp"

#include "drape/bidi.hpp"
#include "drape/drape_tests/img.hpp"
#include "drape/glyph_manager.hpp"

#include "platform/platform.hpp"

#include <QtGui/QPainter>

#include "qt_tstfrm/test_main_loop.hpp"

#include "std/target_os.hpp"

#include <cstring>
#include <functional>
#include <iostream>
#include <memory>
#include <vector>

using namespace std::placeholders;

namespace
{
class GlyphRenderer
{
  strings::UniString m_toDraw;

public:
  GlyphRenderer()
  {
    dp::GlyphManager::Params args;
    args.m_uniBlocks = "unicode_blocks.txt";
    args.m_whitelist = "fonts_whitelist.txt";
    args.m_blacklist = "fonts_blacklist.txt";
    GetPlatform().GetFontNames(args.m_fonts);

    m_mng = std::make_unique<dp::GlyphManager>(args);
  }

  void SetString(std::string const & s)
  {
    m_toDraw = bidi::log2vis(strings::MakeUniString(s));
  }

  void RenderGlyphs(QPaintDevice * device)
  {
    std::vector<dp::GlyphManager::Glyph> glyphs;
    auto generateGlyph = [this, &glyphs](strings::UniChar c)
    {
      dp::GlyphManager::Glyph g = m_mng->GetGlyph(c, dp::GlyphManager::kDynamicGlyphSize);
      glyphs.push_back(dp::GlyphManager::GenerateGlyph(g, m_mng->GetSdfScale()));
      g.m_image.Destroy();
    };

    for (auto const & ucp : m_toDraw)
      generateGlyph(ucp);

    QPainter painter(device);
    painter.fillRect(QRectF(0.0, 0.0, device->width(), device->height()), Qt::white);

    QPoint pen(100, 100);
    float const ratio = 2.0;
    for (auto & g : glyphs)
    {
      if (!g.m_image.m_data)
        continue;

      uint8_t * d = SharedBufferManager::GetRawPointer(g.m_image.m_data);

      QPoint currentPen = pen;
      currentPen.rx() += g.m_metrics.m_xOffset * ratio;
      currentPen.ry() -= g.m_metrics.m_yOffset * ratio;
      painter.drawImage(currentPen, CreateImage(g.m_image.m_width, g.m_image.m_height, d),
                        QRect(0, 0, g.m_image.m_width, g.m_image.m_height));
      pen.rx() += g.m_metrics.m_xAdvance * ratio;
      pen.ry() += g.m_metrics.m_yAdvance * ratio;

      g.m_image.Destroy();
    }
  }

private:
  std::unique_ptr<dp::GlyphManager> m_mng;
};
}  // namespace

UNIT_TEST(GlyphLoadingTest)
{
  // This unit test creates window so can't be run in GUI-less Linux machine.
#ifndef OMIM_OS_LINUX
  GlyphRenderer renderer;

  renderer.SetString("ØŒÆ");
  RunTestLoop("Test1", std::bind(&GlyphRenderer::RenderGlyphs, &renderer, _1));

  renderer.SetString("الحلّة");
  RunTestLoop("Test2", std::bind(&GlyphRenderer::RenderGlyphs, &renderer, _1));

  renderer.SetString("گُلها");
  RunTestLoop("Test3", std::bind(&GlyphRenderer::RenderGlyphs, &renderer, _1));

  renderer.SetString("മനക്കലപ്പടി");
  RunTestLoop("Test4", std::bind(&GlyphRenderer::RenderGlyphs, &renderer, _1));
#endif
}

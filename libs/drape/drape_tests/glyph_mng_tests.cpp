#include "testing/testing.hpp"

#include "drape/drape_tests/img.hpp"

#include "drape/font_constants.hpp"
#include "drape/glyph_manager.hpp"
#include "drape/harfbuzz_shaping.hpp"

#include "base/file_name_utils.hpp"

#include "qt_tstfrm/test_main_loop.hpp"

#include "platform/platform.hpp"

#include <QtGui/QPainter>

#include <algorithm>
#include <functional>
#include <iostream>
#include <numeric>
#include <set>
#include <string>
#include <vector>

#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_MODULE_H
#include <hb-ft.h>

namespace glyph_mng_tests
{
class GlyphRenderer
{
  FT_Library m_freetypeLibrary;
  std::string m_utf8;
  int m_fontPixelSize;
  char const * m_lang;

  static constexpr FT_Int kSdfSpread{dp::kSdfBorder};

public:
  GlyphRenderer()
  {
    // Initialize FreeType
    TEST_EQUAL(0, FT_Init_FreeType(&m_freetypeLibrary), ("Can't initialize FreeType"));
    for (auto const module : {"sdf", "bsdf"})
      TEST_EQUAL(0, FT_Property_Set(m_freetypeLibrary, module, "spread", &kSdfSpread), ());

    dp::GlyphManager::Params args;
    args.m_uniBlocks = base::JoinPath("fonts", "unicode_blocks.txt");
    args.m_whitelist = base::JoinPath("fonts", "whitelist.txt");
    args.m_blacklist = base::JoinPath("fonts", "blacklist.txt");
    GetPlatform().GetFontNames(args.m_fonts);

    m_mng = std::make_unique<dp::GlyphManager>(args);
  }

  ~GlyphRenderer() { FT_Done_FreeType(m_freetypeLibrary); }

  void SetString(std::string const & s, int fontPixelSize, char const * lang)
  {
    m_utf8 = s;
    m_fontPixelSize = fontPixelSize;
    m_lang = lang;
  }

  static float Smoothstep(float edge0, float edge1, float x)
  {
    x = std::clamp((x - edge0) / (edge1 - edge0), 0.0f, 1.0f);
    return x * x * (3 - 2 * x);
  }

  static float PixelColorFromDistance(float distance)
  {
    // float const normalizedDistance = (distance - 128.f) / 128.f;
    float const normalizedDistance = distance / 255.f;
    static constexpr float kFontScale = 1.f;
    static constexpr float kSmoothing = 0.25f / (kSdfSpread * kFontScale);
    float const alpha = Smoothstep(0.5f - kSmoothing, 0.5f + kSmoothing, normalizedDistance);
    return 255.f * alpha;
  }

  void RenderGlyphs(QPaintDevice * device) const
  {
    QPainter painter(device);
    painter.fillRect(QRectF(0.0, 0.0, device->width(), device->height()), Qt::white);

    auto const shapedText = m_mng->ShapeText(m_utf8, m_lang);

    std::cout << "Total width: " << shapedText.m_lineWidthInPixels << '\n';
    std::cout << "Max height: " << shapedText.m_maxLineHeightInPixels << '\n';

    int constexpr kLineStartX = 10;
    int constexpr kLineMarginY = 50;
    QPoint pen(kLineStartX, kLineMarginY);

    for (auto const & glyph : shapedText.m_glyphs)
    {
      constexpr bool kUseSdfBitmap = false;
      dp::GlyphImage img = m_mng->GetGlyphImage(glyph.m_key, kUseSdfBitmap);

      auto const w = img.m_width;
      auto const h = img.m_height;
      // Spaces do not have images.
      if (w && h)
      {
        QPoint currentPen = pen;
        currentPen.rx() += glyph.m_xOffset;
        // Image is drawn at the top left origin, text metrics returns bottom left origin.
        currentPen.ry() -= glyph.m_yOffset + h;
        painter.drawImage(currentPen, CreateImage(w, h, img.m_data->data()), QRect(0, 0, w, h));
      }
      pen += QPoint(glyph.m_xAdvance, glyph.m_yAdvance /* 0 for horizontal texts */);
    }

    pen.rx() = kLineStartX;
    pen.ry() += kLineMarginY;

    for (auto const & glyph : shapedText.m_glyphs)
    {
      constexpr bool kUseSdfBitmap = true;
      auto img = m_mng->GetGlyphImage(glyph.m_key, kUseSdfBitmap);

      auto const w = img.m_width;
      auto const h = img.m_height;
      // Spaces do not have images.
      if (w && h)
      {
        QPoint currentPen = pen;
        currentPen.rx() += glyph.m_xOffset;
        currentPen.ry() -= glyph.m_yOffset + h;
        painter.drawImage(currentPen, CreateImage(w, h, img.m_data->data()),
                          QRect(dp::kSdfBorder, dp::kSdfBorder, w - 2 * dp::kSdfBorder, h - 2 * dp::kSdfBorder));
      }
      pen += QPoint(glyph.m_xAdvance, glyph.m_yAdvance /* 0 for horizontal texts */);
    }

    ////////////////////////////////////////////////////////////////////////////
    // Manual rendering using HB functions.
    {
      pen.rx() = kLineStartX;
      pen.ry() += kLineMarginY;

      auto const hbLanguage = hb_language_from_string(m_lang, -1);

      auto const runs = harfbuzz_shaping::GetTextSegments(m_utf8);
      for (auto const & segment : runs.m_segments)
      {
        hb_buffer_t * buf = hb_buffer_create();
        hb_buffer_add_utf16(buf, reinterpret_cast<uint16_t const *>(runs.m_text.data()), runs.m_text.size(),
                            segment.m_start, segment.m_length);
        hb_buffer_set_direction(buf, segment.m_direction);
        hb_buffer_set_script(buf, segment.m_script);
        hb_buffer_set_language(buf, hbLanguage);

        // If direction, script, and language are not known.
        // hb_buffer_guess_segment_properties(buf);

        std::string const lang = m_lang;
        std::string const fontFileName = lang == "ar" ? "00_NotoNaskhArabic-Regular.ttf"
                                       : lang == "hi" ? "00_NotoSerifDevanagari-Regular.ttf"
                                                      : "07_roboto_medium.ttf";

        auto reader = GetPlatform().GetReader("fonts/" + fontFileName);
        auto fontFile = reader->GetName();
        FT_Face face;
        if (FT_New_Face(m_freetypeLibrary, fontFile.c_str(), 0, &face))
        {
          std::cerr << "Can't load font " << fontFile << '\n';
          return;
        }
        // Set character size
        FT_Set_Pixel_Sizes(face, 0, m_fontPixelSize);
        // This also works.
        // if (FT_Set_Char_Size(face, 0, m_fontPixelSize << 6, 0, 0)) {
        //   std::cerr << "Can't set character size\n";
        //   return;
        // }

        // Set no transform (identity)
        // FT_Set_Transform(face, nullptr, nullptr);

        // Load font into HarfBuzz
        hb_font_t * font = hb_ft_font_create(face, nullptr);

        // Shape!
        hb_shape(font, buf, nullptr, 0);

        // Get the glyph and position information.
        unsigned int glyph_count;
        hb_glyph_info_t * glyph_info = hb_buffer_get_glyph_infos(buf, &glyph_count);
        hb_glyph_position_t * glyph_pos = hb_buffer_get_glyph_positions(buf, &glyph_count);

        for (unsigned int i = 0; i < glyph_count; i++)
        {
          hb_codepoint_t const glyphid = glyph_info[i].codepoint;

          FT_Int32 const flags = FT_LOAD_RENDER;
          FT_Load_Glyph(face, glyphid, flags);
          FT_Render_Glyph(face->glyph, FT_RENDER_MODE_SDF);

          FT_GlyphSlot slot = face->glyph;

          FT_Bitmap const & ftBitmap = slot->bitmap;

          auto const buffer = ftBitmap.buffer;
          auto const width = ftBitmap.width;
          auto const height = ftBitmap.rows;

          for (unsigned h = 0; h < height; ++h)
          {
            for (unsigned w = 0; w < width; ++w)
            {
              auto curPixelAddr = buffer + h * width + w;
              float currPixel = *curPixelAddr;
              currPixel = PixelColorFromDistance(currPixel);
              *curPixelAddr = static_cast<unsigned char>(currPixel);
            }
          }

          auto const bearing_x = slot->metrics.horiBearingX;  // slot->bitmap_left;
          auto const bearing_y = slot->metrics.horiBearingY;  // slot->bitmap_top;

          auto const & glyphPos = glyph_pos[i];
          hb_position_t const x_offset = (glyphPos.x_offset + bearing_x) >> 6;
          hb_position_t const y_offset = (glyphPos.y_offset + bearing_y) >> 6;
          hb_position_t const x_advance = glyphPos.x_advance >> 6;
          hb_position_t const y_advance = glyphPos.y_advance >> 6;

          // Empty images are possible for space characters.
          if (width != 0 && height != 0)
          {
            QPoint currentPen = pen;
            currentPen.rx() += x_offset;
            currentPen.ry() -= y_offset;
            painter.drawImage(currentPen, CreateImage(width, height, buffer),
                              QRect(kSdfSpread, kSdfSpread, width - 2 * kSdfSpread, height - 2 * kSdfSpread));
          }
          pen += QPoint(x_advance, y_advance);
        }

        // Tidy up.
        hb_buffer_destroy(buf);
        hb_font_destroy(font);
        FT_Done_Face(face);
      }
    }

    //////////////////////////////////////////////////////////////////
    // QT text renderer.
    {
      pen.rx() = kLineStartX;
      pen.ry() += kLineMarginY;

      // QFont font("Noto Naskh Arabic");
      QFont font("Roboto");
      font.setPixelSize(m_fontPixelSize);
      font.setWeight(QFont::Weight::Normal);
      painter.setFont(font);
      painter.setPen(Qt::black);
      painter.drawText(pen, "Qt renderer: " + QString::fromUtf8(m_utf8.c_str(), m_utf8.size()));
    }
  }

private:
  std::unique_ptr<dp::GlyphManager> m_mng;
};

// This unit test creates a window so can't be run in GUI-less Linux machine.
// Make sure that the QT_QPA_PLATFORM=offscreen environment variable is set.
UNIT_TEST(GlyphLoadingTest)
{
  GlyphRenderer renderer;

  using namespace std::placeholders;

  constexpr int fontSize = 27;

  // TODO: U+200D/U+200C and other inherited/default-ignorable shaping controls should inherit
  // the surrounding font run. This sample currently shows the broken Drape output above the
  // manually-shaped HarfBuzz reference row.
  renderer.SetString("क्‍ष  क्ष  क्‌ष", fontSize, "hi");
  RunTestLoop("TODO Devanagari ZWJ/ZWNJ shaping", std::bind(&GlyphRenderer::RenderGlyphs, &renderer, _1));

  renderer.SetString("🚻💳♿🛜🚰🚱🚍🚊🚇⛴🚆🚡🏍", fontSize, "en");
  RunTestLoop("Emoji", std::bind(&GlyphRenderer::RenderGlyphs, &renderer, _1));

  renderer.SetString("Muḩāfaz̧at", fontSize, "en");
  RunTestLoop("Latin Extended", std::bind(&GlyphRenderer::RenderGlyphs, &renderer, _1));

  renderer.SetString("Строка", fontSize, "ru");
  RunTestLoop("ru", std::bind(&GlyphRenderer::RenderGlyphs, &renderer, _1));

  renderer.SetString("ØŒÆ", fontSize, "en");
  RunTestLoop("en", std::bind(&GlyphRenderer::RenderGlyphs, &renderer, _1));

  renderer.SetString("𫝚 𫝛 𫝜", fontSize, "zh");
  RunTestLoop("CJK Surrogates", std::bind(&GlyphRenderer::RenderGlyphs, &renderer, _1));

  renderer.SetString(
      "الحلّة گلها"
      " كسول الزنجبيل القط"
      "56"
      "عين علي (الحربية)"
      "123"
      " اَلْعَرَبِيَّةُ",
      fontSize, "ar");
  RunTestLoop("Arabic1", std::bind(&GlyphRenderer::RenderGlyphs, &renderer, _1));

  renderer.SetString(
      "12345"
      "گُلها"
      "12345"
      "گُلها"
      "12345",
      fontSize, "ar");
  RunTestLoop("Arabic2", std::bind(&GlyphRenderer::RenderGlyphs, &renderer, _1));

  renderer.SetString("മനക്കലപ്പടി", fontSize, "ml");
  RunTestLoop("Malay", std::bind(&GlyphRenderer::RenderGlyphs, &renderer, _1));

  renderer.SetString(
      "Test 12 345 "
      "گُلها"
      "678 9000 Test",
      fontSize, "ar");
  RunTestLoop("Arabic Mixed", std::bind(&GlyphRenderer::RenderGlyphs, &renderer, _1));

  renderer.SetString("NFKC Razdoĺny NFKD Razdoĺny", fontSize, "be");
  RunTestLoop("Polish", std::bind(&GlyphRenderer::RenderGlyphs, &renderer, _1));
}

namespace
{
dp::GlyphManager MakeGlyphManager()
{
  dp::GlyphManager::Params args;
  args.m_uniBlocks = base::JoinPath("fonts", "unicode_blocks.txt");
  args.m_whitelist = base::JoinPath("fonts", "whitelist.txt");
  args.m_blacklist = base::JoinPath("fonts", "blacklist.txt");
  GetPlatform().GetFontNames(args.m_fonts);
  return dp::GlyphManager{args};
}

int32_t SumAdvances(dp::text::TextMetrics const & m)
{
  return std::accumulate(m.m_glyphs.begin(), m.m_glyphs.end(), int32_t{0},
                         [](int32_t acc, dp::text::GlyphMetrics const & g) { return acc + g.m_xAdvance; });
}

size_t CountNotdefGlyphs(dp::text::TextMetrics const & m, int fontIndex)
{
  size_t count = 0;
  for (auto const & glyph : m.m_glyphs)
    if (glyph.m_key.m_fontIndex == fontIndex && glyph.m_key.m_glyphId == 0)
      ++count;
  return count;
}
}  // namespace

// Pins the line-width invariant: m_lineWidthInPixels == sum(xAdvance) - first_glyph.m_xOffset.
// The visually-leftmost glyph (first in m_glyphs) carries the line's leading bearing; the bearing
// is subtracted exactly once, regardless of how many segments or font runs the text was split into.
// Regression coverage for the multi-font-run shaping introduced in commit a8a4e9b8.
UNIT_TEST(ShapeText_LineWidthInvariant)
{
  auto mng = MakeGlyphManager();

  // Single LTR segment, single font.
  {
    auto const m = mng.ShapeText("Hello", "en");
    TEST(!m.m_glyphs.empty(), ());
    TEST_EQUAL(m.m_lineWidthInPixels, SumAdvances(m) - m.m_glyphs.front().m_xOffset,
               ("Single LTR run: width = sum(advance) - first bearing"));
  }

  // Multi-segment LTR (Latin + CJK): each script becomes its own segment, so the cross-segment
  // accumulator path is exercised. Bearing must be subtracted only on the visually-first glyph.
  {
    auto const m = mng.ShapeText("Hello 中文", "en");
    TEST(!m.m_glyphs.empty(), ());
    TEST_EQUAL(m.m_lineWidthInPixels, SumAdvances(m) - m.m_glyphs.front().m_xOffset,
               ("Multi-segment LTR: width invariant across segment boundaries"));

    // Visually-leftmost glyph is from the Latin font (first physical character is 'H').
    auto const latinFont = mng.GetFontIndex(static_cast<strings::UniChar>('H'));
    TEST_NOT_EQUAL(latinFont, -1, ());
    TEST_EQUAL(m.m_glyphs.front().m_key.m_fontIndex, latinFont,
               ("First emitted glyph should be from the visually-leftmost run's font"));
  }

  // Single-segment, multi-font run path: a Latin letter followed by an emoji forces the per-font
  // run split inside one segment after the emoji-fallback work in commit a8a4e9b8.
  {
    auto const m = mng.ShapeText("a🚻", "en");
    TEST(!m.m_glyphs.empty(), ());
    TEST_EQUAL(m.m_lineWidthInPixels, SumAdvances(m) - m.m_glyphs.front().m_xOffset,
               ("Single-segment multi-font: width invariant within a segment"));

    std::set<int> fontIndices;
    for (auto const & g : m.m_glyphs)
      fontIndices.insert(g.m_key.m_fontIndex);
    TEST_GREATER(fontIndices.size(), size_t{1}, ("Latin + emoji must come from different fonts"));
  }

  // Pure RTL: HB emits glyphs in visual L-to-R order, so the first emitted glyph is the
  // visually-leftmost of the line and must be the one whose bearing is subtracted.
  {
    auto const m = mng.ShapeText("مرحبا", "ar");
    TEST(m.m_isRTL, ());
    TEST(!m.m_glyphs.empty(), ());
    TEST_EQUAL(m.m_lineWidthInPixels, SumAdvances(m) - m.m_glyphs.front().m_xOffset,
               ("RTL: width invariant holds with reversed-run shaping"));
  }

  // Bidi LTR + RTL: the LTR segment renders first (visually-leftmost), so the very first emitted
  // glyph is from the Latin font and the line-width invariant must still hold globally.
  {
    auto const m = mng.ShapeText("Hello مرحبا", "en");
    TEST(!m.m_glyphs.empty(), ());
    TEST_EQUAL(m.m_lineWidthInPixels, SumAdvances(m) - m.m_glyphs.front().m_xOffset,
               ("Bidi LTR+RTL: width invariant holds across direction changes"));

    auto const latinFont = mng.GetFontIndex(static_cast<strings::UniChar>('H'));
    TEST_EQUAL(m.m_glyphs.front().m_key.m_fontIndex, latinFont,
               ("LTR-dominant bidi: visually-leftmost glyph is from the Latin font"));
  }
}

// A codepoint for which GetFontIndex returns kInvalidFont (no font in the stack supports it)
// must stay in an available font run. HB then emits glyph id 0 (.notdef), so the label shows
// an explicit missing-glyph box instead of silently dropping the original character.
//
// U+AB30 LATIN SMALL LETTER BARRED ALPHA (Latin Extended-E): the block is missing from
// data/fonts/unicode_blocks.txt, so GetFontIndex returns kInvalidFont. ICU classifies it as
// Latin script, so GetSingleTextLineRuns groups it into the same segment as the following
// ASCII text -- exactly the situation that exercises the per-font-run loop.
UNIT_TEST(ShapeText_NoFontCodepointEmitsNotdef)
{
  auto mng = MakeGlyphManager();

  static constexpr strings::UniChar kNoFontCp = 0xAB30;
  TEST_EQUAL(mng.GetFontIndex(kNoFontCp), -1, ("U+AB30 must be unmapped for this test to be meaningful"));

  static constexpr char const * kNoFontUtf8 = "\xEA\xAC\xB0";  // U+AB30
  std::string const plain = "Hello";

  auto const baseline = mng.ShapeText(plain, "en");
  auto const latinFont = mng.GetFontIndex(static_cast<strings::UniChar>('H'));
  TEST_NOT_EQUAL(latinFont, -1, ());

  // Leading: codepoint precedes all visible text.
  {
    auto const m = mng.ShapeText(std::string(kNoFontUtf8) + plain, "en");
    TEST_EQUAL(baseline.m_glyphs.size() + 1, m.m_glyphs.size(), ("Leading unmapped codepoint emits .notdef"));
    TEST_EQUAL(m.m_glyphs.front().m_key.m_fontIndex, latinFont, ());
    TEST_EQUAL(m.m_glyphs.front().m_key.m_glyphId, 0, ());
  }

  // Trailing: codepoint follows all visible text.
  {
    auto const m = mng.ShapeText(plain + kNoFontUtf8, "en");
    TEST_EQUAL(baseline.m_glyphs.size() + 1, m.m_glyphs.size(), ("Trailing unmapped codepoint emits .notdef"));
    TEST_EQUAL(m.m_glyphs.back().m_key.m_fontIndex, latinFont, ());
    TEST_EQUAL(m.m_glyphs.back().m_key.m_glyphId, 0, ());
  }

  // Inner: codepoint sits between two same-font ASCII characters and stays in that run.
  {
    auto const m = mng.ShapeText("Hel" + std::string(kNoFontUtf8) + "lo", "en");
    TEST_EQUAL(baseline.m_glyphs.size() + 1, m.m_glyphs.size(), ("Inner unmapped codepoint emits .notdef"));
    TEST_EQUAL(CountNotdefGlyphs(m, latinFont), size_t{1}, ());
  }

  // Only unmapped text has no surrounding run, so it falls back to the default font's .notdef.
  {
    auto const m = mng.ShapeText(kNoFontUtf8, "en");
    TEST_EQUAL(m.m_glyphs.size(), size_t{1}, ("Only unmapped codepoint emits .notdef"));
    TEST_NOT_EQUAL(m.m_glyphs.front().m_key.m_fontIndex, -1, ());
    TEST_EQUAL(m.m_glyphs.front().m_key.m_glyphId, 0, ());
  }
}

// Pins basic positivity / monotonicity invariants. Catches gross regressions from the shaping
// rewrite without depending on exact font metrics.
UNIT_TEST(ShapeText_BasicInvariants)
{
  auto mng = MakeGlyphManager();

  // Width is strictly positive for non-empty visible text.
  TEST_GREATER(mng.ShapeText("a", "en").m_lineWidthInPixels, 0, ());

  // Width grows when characters are appended.
  auto const w1 = mng.ShapeText("a", "en").m_lineWidthInPixels;
  auto const w2 = mng.ShapeText("ab", "en").m_lineWidthInPixels;
  auto const w3 = mng.ShapeText("abc", "en").m_lineWidthInPixels;
  TEST_GREATER(w2, w1, ());
  TEST_GREATER(w3, w2, ());

  // Cache returns equivalent metrics for identical inputs.
  auto const a = mng.ShapeText("CacheKey", "en");
  auto const b = mng.ShapeText("CacheKey", "en");
  TEST_EQUAL(a.m_lineWidthInPixels, b.m_lineWidthInPixels, ());
  TEST_EQUAL(a.m_glyphs.size(), b.m_glyphs.size(), ());
  TEST_EQUAL(a.m_isRTL, b.m_isRTL, ());
}

// Exercises the LRU cap path: inserts more unique entries than the cache holds, then verifies
// (a) the run completes without crashing -- proves that LRU eviction does not corrupt the index
// or list iterators, and (b) both a hot entry (still cached) and a cold entry (evicted, must be
// recomputed) return identical metrics on a repeat shape, proving cache-miss recomputation is
// consistent with cache-hit lookup.
//
// Regression coverage for the wholesale clear() that the LRU replaced -- if an LRU bug ever
// dropped recent entries or held on to stale iterators, this test would crash or diverge.
UNIT_TEST(ShapeText_CacheBoundedUnderLoad)
{
  auto mng = MakeGlyphManager();

  // Cache cap is 20000 internally; insert clearly past it to force eviction.
  size_t constexpr kInsertCount = 22000;

  for (size_t i = 0; i < kInsertCount; ++i)
  {
    auto const s = "label_" + std::to_string(i);
    auto const m = mng.ShapeText(s, "en");
    TEST(!m.m_glyphs.empty(), (i));
  }

  // Recent entry (last inserted): expected to still be cached. Hit path after many evictions.
  auto const recent = "label_" + std::to_string(kInsertCount - 1);
  auto const m1 = mng.ShapeText(recent, "en");
  auto const m2 = mng.ShapeText(recent, "en");
  TEST_EQUAL(m1.m_lineWidthInPixels, m2.m_lineWidthInPixels, ());
  TEST_EQUAL(m1.m_glyphs.size(), m2.m_glyphs.size(), ());

  // Oldest entry: expected to be evicted; second call recomputes. Both shapes must agree.
  auto const oldest = std::string{"label_0"};
  auto const m3 = mng.ShapeText(oldest, "en");
  auto const m4 = mng.ShapeText(oldest, "en");
  TEST_EQUAL(m3.m_lineWidthInPixels, m4.m_lineWidthInPixels, ());
  TEST_EQUAL(m3.m_glyphs.size(), m4.m_glyphs.size(), ());
}

// Pins the per-glyph readiness tracking that TextHandle::Update polls until the atlas warms up.
// Regression coverage for the std::set -> std::bitset switch in m_readyGlyphs: marking must be
// observable by AreGlyphsReady, idempotent, and isolated per-key. Crucially, m_readyGlyphs is
// per-Font: marking a glyph in font A must not appear ready in font B even at the same glyph id.
UNIT_TEST(GlyphManager_GlyphReadinessTracking)
{
  auto mng = MakeGlyphManager();

  // Latin + emoji crosses fonts (DejaVu Sans + organic_maps_emoji): the keys we pick below come
  // from different m_fontIndex values, so the test exercises per-Font isolation in addition to
  // the bitset's set/test logic.
  auto const shaped = mng.ShapeText("a🚻", "en");
  TEST_GREATER_OR_EQUAL(shaped.m_glyphs.size(), size_t{2}, ("need at least two glyphs to test"));

  // Pick one Latin glyph and one emoji glyph so k1.m_fontIndex != k2.m_fontIndex.
  auto const & latinKey = shaped.m_glyphs.front().m_key;
  auto const emojiIt = std::find_if(shaped.m_glyphs.begin(), shaped.m_glyphs.end(),
                                    [&](auto const & g) { return g.m_key.m_fontIndex != latinKey.m_fontIndex; });
  TEST(emojiIt != shaped.m_glyphs.end(), ("Latin + emoji must come from different fonts"));

  dp::GlyphFontAndId const k1 = latinKey;
  dp::GlyphFontAndId const k2 = emojiIt->m_key;
  TEST_NOT_EQUAL(k1.m_fontIndex, k2.m_fontIndex, ("test relies on cross-font keys"));

  dp::TGlyphs const both{k1, k2};
  dp::TGlyphs const justK1{k1};
  dp::TGlyphs const justK2{k2};

  // Initial state: nothing marked ready.
  TEST(!mng.AreGlyphsReady(both), ("no glyphs marked yet"));
  TEST(!mng.AreGlyphsReady(justK1), ("k1 not marked yet"));

  mng.MarkGlyphReady(k1);
  TEST(mng.AreGlyphsReady(justK1), ("k1 ready after mark"));
  TEST(!mng.AreGlyphsReady(justK2), ("k2 still not ready -- per-Font bitset must not bleed"));
  TEST(!mng.AreGlyphsReady(both), ("k2 still missing in combined set"));

  // Cross-font isolation pin: a foreign key with k1.m_glyphId in k2's font must NOT register as
  // ready just because (k1.m_fontIndex, k1.m_glyphId) was marked.
  dp::GlyphFontAndId const foreign{k2.m_fontIndex, k1.m_glyphId};
  TEST(!mng.AreGlyphsReady(dp::TGlyphs{foreign}), ("readiness must be isolated per font"));

  mng.MarkGlyphReady(k2);
  TEST(mng.AreGlyphsReady(both), ("both ready after marking k2"));

  // Idempotency: re-marking must not flip a previously-ready glyph back.
  mng.MarkGlyphReady(k1);
  mng.MarkGlyphReady(k2);
  TEST(mng.AreGlyphsReady(both), ("repeated marks are idempotent"));
}

}  // namespace glyph_mng_tests

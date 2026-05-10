#include "drape/glyph_manager.hpp"

#include "drape/font_constants.hpp"
#include "drape/glyph.hpp"
#include "drape/harfbuzz_shaping.hpp"

#include "platform/platform.hpp"
#include "platform/preferred_languages.hpp"

#include "coding/hex.hpp"
#include "coding/reader.hpp"
#include "coding/reader_streambuf.hpp"
#include "coding/string_utf8_multilang.hpp"

#include "base/checked_cast.hpp"
#include "base/internal/message.hpp"
#include "base/logging.hpp"
#include "base/macros.hpp"
#include "base/stl_helpers.hpp"
#include "base/string_utils.hpp"

#include <ft2build.h>
#include <hb-ft.h>
#include <algorithm>
#include <array>
#include <bitset>
#include <cstdint>
#include <cstring>
#include <istream>
#include <limits>
#include <list>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_set>
#include <utility>
#include <vector>

#include <boost/unordered/unordered_flat_map.hpp>

#include FT_FREETYPE_H
#include FT_MODULE_H
#include FT_SYSTEM_H
#include FT_SIZES_H
#include FT_TYPES_H

#undef __FTERRORS_H__
#define FT_ERRORDEF(e, v, s) {e, s},
#define FT_ERROR_START_LIST  {
#define FT_ERROR_END_LIST    {0, 0}}
namespace
{
struct FreetypeError
{
  int m_code;
  char const * const m_message;
};

// clang-format off
FreetypeError constexpr g_FT_Errors[] =
#include FT_ERRORS_H
;
// clang-format on

char const * GetFreetypeErrorMessage(FT_Error code)
{
  for (auto const & error : g_FT_Errors)
  {
    if (error.m_message == nullptr)
      break;
    if (error.m_code == code)
      return error.m_message;
  }

  return "unknown error";
}

FT_Error CheckFreetype(FT_Error error, char const * expression)
{
  if (error != 0)
    LOG(LERROR, ("FreeType call failed:", expression, error, GetFreetypeErrorMessage(error)));
  return error;
}
}  // namespace

#define FREETYPE_CHECK(x) CheckFreetype((x), #x)

namespace
{
// RAII owner for an FT_Face. Releases via FT_Done_Face on destruction, including the
// stack-unwind path when a Font constructor throws after FT_Open_Face has succeeded but
// before the destructor becomes reachable.
struct FtFaceDeleter
{
  void operator()(FT_FaceRec_ * face) const noexcept
  {
    if (face != nullptr)
      FREETYPE_CHECK(FT_Done_Face(face));
  }
};
using FtFacePtr = std::unique_ptr<FT_FaceRec_, FtFaceDeleter>;

// RAII owner for an hb_font_t. Closes the unwind window between hb_ft_font_create and the
// rest of Font's constructor (e.g. m_glyphMetricsCache.resize bad_alloc on a CJK font).
struct HbFontDeleter
{
  void operator()(hb_font_t * font) const noexcept
  {
    if (font != nullptr)
      hb_font_destroy(font);
  }
};
using HbFontPtr = std::unique_ptr<hb_font_t, HbFontDeleter>;
}  // namespace

namespace dp
{
int constexpr kInvalidFont = -1;

// Sentinel value used in font white/black-list entries to apply a rule to every unicode block.
constexpr std::string_view kAllBlocks = "*";

namespace
{
void HashCombine(size_t & seed, size_t value)
{
  seed ^= value + 0x9e3779b97f4a7c15ULL + (seed << 6) + (seed >> 2);
}

struct TextMetricsCacheKeyView
{
  std::string_view m_utf8;
  int8_t m_lang;
};

struct TextMetricsCacheKey
{
  std::string m_utf8;
  int8_t m_lang;
};

size_t HashTextMetricsCacheKey(TextMetricsCacheKeyView const & key)
{
  size_t seed = base::StringHash{}(key.m_utf8);
  HashCombine(seed, std::hash<int>{}(key.m_lang));
  return seed;
}

struct TextMetricsCacheKeyHash
{
  using is_transparent = void;

  size_t operator()(TextMetricsCacheKey const & key) const { return HashTextMetricsCacheKey({key.m_utf8, key.m_lang}); }

  size_t operator()(TextMetricsCacheKeyView const & key) const { return HashTextMetricsCacheKey(key); }
};

struct TextMetricsCacheKeyEqual
{
  using is_transparent = void;

  static bool Equal(TextMetricsCacheKeyView const & lhs, TextMetricsCacheKeyView const & rhs)
  {
    return lhs.m_lang == rhs.m_lang && lhs.m_utf8 == rhs.m_utf8;
  }

  bool operator()(TextMetricsCacheKey const & lhs, TextMetricsCacheKey const & rhs) const
  {
    return Equal({lhs.m_utf8, lhs.m_lang}, {rhs.m_utf8, rhs.m_lang});
  }

  bool operator()(TextMetricsCacheKey const & lhs, TextMetricsCacheKeyView const & rhs) const
  {
    return Equal({lhs.m_utf8, lhs.m_lang}, rhs);
  }

  bool operator()(TextMetricsCacheKeyView const & lhs, TextMetricsCacheKey const & rhs) const
  {
    return Equal(lhs, {rhs.m_utf8, rhs.m_lang});
  }
};

struct FontRun
{
  int32_t m_start = 0;
  int32_t m_length = 0;
  int m_fontIndex = kInvalidFont;
};

// Bounded LRU for shaped text metrics. Replaces the previous design that called clear() at the
// cap, dropping the entire ~80%-hit-rate cache and producing a multi-second jank as the working
// set rebuilt. Splice-on-hit keeps recently-used entries; pop_back evicts the least-recently-used
// when over the cap.
//
// Mirrors base/lru_cache.hpp::LruCache structurally (list + hash-map, splice-on-hit) but adds
// two capabilities the hot path needs:
//   1. Heterogeneous (transparent) lookup -- Find takes a string_view-keyed view so we do not
//      allocate a std::string for the ~80% of calls that hit. ShapeText is called per label
//      per frame; the per-call allocation would be measurable malloc traffic.
//   2. Find returns nullptr on miss; the caller does HarfBuzz shaping outside the cache and
//      commits via Insert. LruCache::Find creates a default-constructed Value slot eagerly,
//      so an empty TextMetrics would sit in the cache during shaping.
// Worth folding back into base/lru_cache.hpp once that gains transparent-lookup support and a
// Find/Insert split.
class TextMetricsCache
{
public:
  // Default cap of 20000 covers the working set of a typical browse session while keeping the
  // cache footprint bounded after TextMetrics gained an inline glyph buffer (~368 B per entry,
  // up from ~40 B with the previous std::vector field). At 20k entries the worst-case resident
  // set is ~7.4 MB, vs ~18.4 MB at the prior 50k cap.
  // maxSize must be >= 1 so Insert can always retain the entry it just added; a zero cap would
  // evict the new node before Insert returns its m_value reference, dangling the caller.
  explicit TextMetricsCache(size_t maxSize = 20000) : m_maxSize(maxSize) { CHECK_GREATER(maxSize, 0, ()); }

  // Returns nullptr on miss. On hit, moves the entry to MRU position. The returned pointer is
  // valid until the next Find/Insert call on this cache (subsequent ops may evict it).
  text::TextMetrics const * Find(TextMetricsCacheKeyView const & key)
  {
    auto const found = m_index.find(key);
    if (found == m_index.end())
      return nullptr;

    // splice() to the same list with src == dst is a no-op per the standard; the unconditional
    // call is fine and saves a branch.
    m_lru.splice(m_lru.begin(), m_lru, found->second);
    return &found->second->m_value;
  }

  // Inserts a new entry at MRU position. Caller must have verified the key isn't already present
  // via a prior Find(); inserting a duplicate is undefined for this cache.
  text::TextMetrics const & Insert(TextMetricsCacheKey key, text::TextMetrics value)
  {
    m_lru.emplace_front(std::move(key), std::move(value));
    auto const inserted = m_lru.begin();
    m_index.emplace(inserted->m_key, inserted);

    while (m_lru.size() > m_maxSize)
    {
      m_index.erase(m_lru.back().m_key);
      m_lru.pop_back();
    }

    return inserted->m_value;
  }

  size_t Size() const { return m_index.size(); }

private:
  struct Node
  {
    Node(TextMetricsCacheKey k, text::TextMetrics v) : m_key(std::move(k)), m_value(std::move(v)) {}
    TextMetricsCacheKey m_key;
    text::TextMetrics m_value;
  };

  size_t m_maxSize;
  std::list<Node> m_lru;  // front = most recently used, back = least recently used
  // boost::unordered_flat_map is open-addressed (one cache line per probe vs std::unordered_map's
  // bucket-list indirection). Measured ~30% lower ns/op on the steady-hit microbenchmark and
  // ~17% lower end-to-end ShapeText (libs/drape/drape_tests/text_metrics_cache_bench.cpp).
  boost::unordered_flat_map<TextMetricsCacheKey, std::list<Node>::iterator, TextMetricsCacheKeyHash,
                            TextMetricsCacheKeyEqual>
      m_index;
};

using CJKVariant = languages::CJKResolver::Variant;

FT_Long PickCJKFaceIndex(std::string const & path, FT_Library lib, CJKVariant want)
{
  // /system/fonts/* are filesystem paths so FT_New_Face mmaps directly — no need to slurp the
  // whole TTC into memory.
  FT_Face probe = nullptr;
  if (FT_New_Face(lib, path.c_str(), 0, &probe) != 0 || probe == nullptr)
  {
    LOG(LWARNING, ("CJK probe failed to open", path));
    return 0;
  }
  FT_Long const numFaces = probe->num_faces;
  FT_Done_Face(probe);

  using Face = std::pair<FT_Long, CJKVariant>;
  std::vector<Face> faces;
  faces.reserve(base::asserted_cast<size_t>(numFaces));

  for (FT_Long i = 0; i < numFaces; ++i)
  {
    FT_Face face = nullptr;
    if (FT_New_Face(lib, path.c_str(), i, &face) != 0 || face == nullptr)
      continue;
    char const * const familyRaw = face->family_name;
    auto const detected =
        familyRaw ? languages::CJKResolver::FromSfntFamilyName(familyRaw) : std::optional<CJKVariant>{};
    LOG(LINFO, ("CJK face", path, "index", i, "family", familyRaw ? familyRaw : "(null)", "->",
                detected ? languages::DebugPrint(*detected) : std::string{"unknown"}));
    if (detected)
      faces.emplace_back(i, *detected);
    FT_Done_Face(face);
  }

  for (auto const variant : languages::CJKResolver::FallbackChain(want))
  {
    auto const it = std::ranges::find(faces, variant, &Face::second);
    if (it != faces.end())
    {
      LOG(LINFO, ("CJK chosen face for", path, "want", languages::DebugPrint(want), "got",
                  languages::DebugPrint(it->second), "->", it->first));
      return it->first;
    }
  }

  LOG(LWARNING, ("CJK no chain match for", path, "fallback to face 0"));
  return 0;
}
}  // namespace

template <typename ToDo>
void ParseUniBlocks(std::string const & uniBlocksFile, ToDo toDo)
{
  try
  {
    ReaderStreamBuf buffer(GetPlatform().GetReader(uniBlocksFile));
    std::istream fin(&buffer);
    while (true)
    {
      std::string name;
      uint32_t start, end;
      fin >> name >> std::hex >> start >> std::hex >> end;
      if (!fin)
        break;

      toDo(name, start, end);
    }
  }
  catch (RootException const & e)
  {
    LOG(LCRITICAL, ("Error reading uniblock description: ", e.what()));
  }
}

template <typename ToDo>
void ParseFontList(std::string const & fontListFile, ToDo toDo)
{
  try
  {
    ReaderStreamBuf buffer(GetPlatform().GetReader(fontListFile));
    std::istream fin(&buffer);
    while (true)
    {
      std::string ubName;
      std::string fontName;
      fin >> ubName >> fontName;
      if (!fin)
        break;

      toDo(ubName, fontName);
    }
  }
  catch (RootException const & e)
  {
    LOG(LWARNING, ("Error reading font list ", fontListFile, " : ", e.what()));
  }
}

class Font
{
public:
  DECLARE_EXCEPTION(InvalidFontException, RootException);
  DISALLOW_COPY_AND_MOVE(Font);

  // faceIndex selects which face inside a TTC collection to open; pass 0 for single-face files.
  Font(ReaderPtr<Reader> && fontReader, FT_Library lib, FT_Long faceIndex = 0) : m_fontReader(std::move(fontReader))
  {
    std::memset(&m_stream, 0, sizeof(m_stream));
    m_stream.size = static_cast<unsigned long>(m_fontReader.Size());
    m_stream.descriptor.pointer = &m_fontReader;
    m_stream.read = &Font::Read;
    m_stream.close = &Font::Close;

    FT_Open_Args args = {};
    args.flags = FT_OPEN_STREAM;
    args.stream = &m_stream;

    // Take ownership immediately so any throw between here and the end of the constructor
    // releases the FT_Face during member-unwind. ~Font is not invoked on partial construction.
    FT_Face raw = nullptr;
    FT_Error const err = FT_Open_Face(lib, &args, faceIndex, &raw);
    m_fontFace.reset(raw);
    if (err || !IsValid())
      MYTHROW(InvalidFontException, (err, err != 0 ? GetFreetypeErrorMessage(err) : "invalid or empty font face"));

    // The same font size is used to render all glyphs to textures and to shape them.
    if (auto const sizeErr = FREETYPE_CHECK(FT_Set_Pixel_Sizes(m_fontFace.get(), 0, kBaseFontSizePixels)); sizeErr != 0)
      MYTHROW(InvalidFontException, (sizeErr, GetFreetypeErrorMessage(sizeErr)));

    if (auto const activateErr = FREETYPE_CHECK(FT_Activate_Size(m_fontFace->size)); activateErr != 0)
      MYTHROW(InvalidFontException, (activateErr, GetFreetypeErrorMessage(activateErr)));

    m_harfbuzzFont.reset(hb_ft_font_create(m_fontFace.get(), nullptr));
    CHECK(m_harfbuzzFont != nullptr, ());

    // Pre-size the per-glyph metrics cache. HB only emits glyph IDs in [0, num_glyphs), so direct
    // indexing is safe. m_loaded defaults to false; entries become populated on first shape.
    //
    // Memory cost is deliberate: 16 B per glyph slot, ~1.55 MB across the bundled font stack
    // and ~3 MB on Android once /system/fonts/ is scanned (NotoSansCJK alone is ~1 MB). A sparse
    // unordered_map would save ~1.5 MB at the cost of hash + bucket-walk on every glyph access
    // during shaping -- the inner loop of every label. Revisit if the stack grows toward full
    // Noto CJK (multiple weights of ~65k glyphs each).
    //
    // resize() can throw bad_alloc on memory-pressured devices; m_harfbuzzFont's deleter
    // releases the HB font during member unwind so the partial-construction path doesn't leak.
    m_glyphMetricsCache.resize(static_cast<size_t>(m_fontFace->num_glyphs));
  }

  ~Font() = default;

  bool IsValid() const { return m_fontFace && m_fontFace->num_glyphs > 0; }

  bool HasGlyph(strings::UniChar unicodePoint) const { return FT_Get_Char_Index(m_fontFace.get(), unicodePoint) != 0; }

  GlyphImage GetGlyphImage(uint16_t glyphId, bool sdf)
  {
    // FT_LOAD_RENDER with FT_RENDER_MODE_SDF uses bsdf driver that is around 3x times faster
    // than sdf driver, activated by FT_LOAD_DEFAULT + FT_RENDER_MODE_SDF
    if (FREETYPE_CHECK(FT_Load_Glyph(m_fontFace.get(), glyphId, FT_LOAD_RENDER)) != 0)
      return {};

    if (sdf)
    {
      if (FREETYPE_CHECK(FT_Render_Glyph(m_fontFace->glyph, FT_RENDER_MODE_SDF)) != 0)
        return {};
    }

    FT_Bitmap const & bitmap = m_fontFace->glyph->bitmap;

    SharedBufferManager::shared_buffer_ptr_t data;
    if (bitmap.buffer != nullptr)
    {
      // Bitmap is stored without a padding.
      data = SharedBufferManager::Instance().ReserveSharedBuffer(bitmap.width * bitmap.rows);
      auto ptr = data->data();

      if (bitmap.pitch == static_cast<int>(bitmap.width))
        std::memcpy(ptr, bitmap.buffer, bitmap.width * bitmap.rows);
      else
        for (unsigned int row = 0; row < bitmap.rows; ++row)
          std::memcpy(ptr + row * bitmap.width, bitmap.buffer + row * bitmap.pitch, bitmap.width);
    }
    return {bitmap.width, bitmap.rows, std::move(data)};
  }

  void GetCharcodes(std::vector<FT_ULong> & charcodes) const
  {
    FT_UInt gindex = 0;
    // FT_Get_First_Char / FT_Get_Next_Char walk the active charmap in strictly ascending codepoint
    // order, so the result is already sorted and unique -- no SortUnique needed.
    auto charcode = FT_Get_First_Char(m_fontFace.get(), &gindex);
    while (gindex)
    {
      charcodes.push_back(charcode);
      charcode = FT_Get_Next_Char(m_fontFace.get(), charcode, &gindex);
    }
  }

  static unsigned long Read(FT_Stream stream, unsigned long offset, unsigned char * buffer, unsigned long count)
  {
    if (count != 0)
    {
      try
      {
        auto * reader = reinterpret_cast<ReaderPtr<Reader> *>(stream->descriptor.pointer);
        reader->Read(offset, buffer, count);
      }
      catch (RootException const & e)
      {
        LOG(LERROR, ("Error reading font stream:", e.what()));
        return 0;
      }
    }

    return count;
  }

  static void Close(FT_Stream) {}

  // MarkGlyphReady runs on backend (during shaping) and render (during UploadResources, called
  // outside GlyphIndex's m_mutex) threads; IsGlyphReady runs on the frontend renderer thread via
  // TextureManager::AreGlyphsReady. The std::bitset's per-bit set/test on aligned words tolerates
  // the race: the worst case is a delayed-visible "ready" bit, which only causes the FR to retry
  // on the next frame -- there is no torn read or use-after-free, unlike the prior std::set design.
  void MarkGlyphReady(uint16_t glyphId) { m_readyGlyphs.set(glyphId); }

  bool IsGlyphReady(uint16_t glyphId) const { return m_readyGlyphs.test(glyphId); }

  // This code is not thread safe.
  void Shape(hb_buffer_t * hbBuffer, int fontIndex, text::TextMetrics & outMetrics)
  {
    hb_shape(m_harfbuzzFont.get(), hbBuffer, nullptr, 0);

    unsigned int glyphCount;
    hb_glyph_info_t const * glyphInfo = hb_buffer_get_glyph_infos(hbBuffer, &glyphCount);
    hb_glyph_position_t const * glyphPos = hb_buffer_get_glyph_positions(hbBuffer, &glyphCount);

    for (unsigned int i = 0; i < glyphCount; ++i)
    {
      // TODO(AB): Check for missing glyph ID?
      auto const glyphId = static_cast<uint16_t>(glyphInfo[i].codepoint);

      auto const & currPos = glyphPos[i];
      int32_t const xAdvance = currPos.x_advance >> 6;

      auto const cached = LoadGlyphMetrics(glyphId);
      if (!cached)
      {
        outMetrics.AddGlyphMetrics(static_cast<int16_t>(fontIndex), glyphId, 0, 0, xAdvance, kBaseFontSizePixels);
        continue;
      }

      auto const xOffset = static_cast<int32_t>((currPos.x_offset + cached->m_horiBearingX) >> 6);
      // The original Drape code expects a bottom, not a top offset in its calculations.
      auto const yOffset = static_cast<int32_t>((currPos.y_offset + cached->m_horiBearingY - cached->m_height) >> 6);
      // yAdvance is always zero for horizontal text layouts.

      outMetrics.AddGlyphMetrics(static_cast<int16_t>(fontIndex), glyphId, xOffset, yOffset, xAdvance,
                                 kBaseFontSizePixels);
    }
  }

private:
  // Cached per-glyph metrics in 26.6 fixed point. Populated lazily on first shape; identical to
  // FT_Load_Glyph(FT_LOAD_DEFAULT)->glyph->metrics so callers see no metric drift.
  struct GlyphMetricsCacheEntry
  {
    int32_t m_horiBearingX = 0;
    int32_t m_horiBearingY = 0;
    int32_t m_height = 0;
    bool m_loaded = false;
  };

  // Returns a pointer into the cache for the given glyph, populating it via FT_Load_Glyph on miss.
  // Returns nullptr when FT_Load_Glyph fails so the caller can emit zero-bearing fallback metrics.
  GlyphMetricsCacheEntry const * LoadGlyphMetrics(uint16_t glyphId)
  {
    ASSERT_LESS(glyphId, m_glyphMetricsCache.size(), ());
    auto & entry = m_glyphMetricsCache[glyphId];
    if (entry.m_loaded)
      return &entry;

    if (FREETYPE_CHECK(FT_Load_Glyph(m_fontFace.get(), glyphId, FT_LOAD_DEFAULT)) != 0)
      return nullptr;

    auto const & metrics = m_fontFace->glyph->metrics;
    entry.m_horiBearingX = static_cast<int32_t>(metrics.horiBearingX);
    entry.m_horiBearingY = static_cast<int32_t>(metrics.horiBearingY);
    entry.m_height = static_cast<int32_t>(metrics.height);
    entry.m_loaded = true;
    return &entry;
  }

  ReaderPtr<Reader> m_fontReader;
  FT_StreamRec_ m_stream;
  FtFacePtr m_fontFace;

  // Glyph IDs are uint16_t (truncated from FT_UInt at the hb_shape boundary above), so the
  // bitset is sized to the full uint16_t value domain. Replaces a std::set<uint16_t> that
  // allocated a tree node per glyph -- O(1) test/set, ~620 KB less heap fragmentation across
  // all loaded fonts, and noticeably better cache behaviour during atlas warm-up when
  // ~thousands of glyphs are queried per frame via TextHandle::Update -> AreGlyphsReady.
  static constexpr size_t kMaxGlyphsPerFont = static_cast<size_t>(std::numeric_limits<uint16_t>::max()) + 1;
  std::bitset<kMaxGlyphsPerFont> m_readyGlyphs;
  std::vector<GlyphMetricsCacheEntry> m_glyphMetricsCache;

  // Declared after m_fontFace so HbFontDeleter runs first: hb_font_destroy releases its FT_Face
  // reference before FtFaceDeleter calls FT_Done_Face.
  HbFontPtr m_harfbuzzFont;
};

// Information about single unicode block.
struct UnicodeBlock
{
  std::string m_name;

  strings::UniChar m_start;
  strings::UniChar m_end;
  std::vector<int> m_fontsWeight;

  UnicodeBlock(std::string name, strings::UniChar start, strings::UniChar end)
    : m_name(std::move(name))
    , m_start(start)
    , m_end(end)
  {}

  int GetFontOffset(int idx) const
  {
    if (m_fontsWeight.empty())
      return kInvalidFont;

    int maxWeight = 0;
    int upperBoundWeight = std::numeric_limits<int>::max();
    if (idx != kInvalidFont)
      upperBoundWeight = m_fontsWeight[idx];

    int index = kInvalidFont;
    ASSERT_LESS(m_fontsWeight.size(), static_cast<size_t>(std::numeric_limits<int>::max()), ());
    for (size_t i = 0; i < m_fontsWeight.size(); ++i)
    {
      int const w = m_fontsWeight[i];
      if (w < upperBoundWeight && w > maxWeight)
      {
        maxWeight = w;
        index = static_cast<int>(i);
      }
    }

    return index;
  }

  bool HasSymbol(strings::UniChar sym) const { return (m_start <= sym) && (m_end >= sym); }
};

using TUniBlocks = std::vector<UnicodeBlock>;
using TUniBlockIter = TUniBlocks::const_iterator;

struct GlyphManager::Impl
{
  DISALLOW_COPY_AND_MOVE(Impl);

  Impl()
  {
    m_harfbuzzBuffer = hb_buffer_create();
    CHECK(m_harfbuzzBuffer != nullptr, ());
  }

  ~Impl()
  {
    m_fonts.clear();
    if (m_library)
      FREETYPE_CHECK(FT_Done_FreeType(m_library));

    hb_buffer_destroy(m_harfbuzzBuffer);
  }

  // Lazy int8_t -> hb_language_t cache. Skips StringUtf8Multilang::GetLangByCode and
  // hb_language_from_string (a string-keyed lookup in HB's global language table) on the
  // common path. HB language pointers are stable singletons so a value cached today is valid
  // forever. ShapeText is the only caller and is single-threaded by contract.
  hb_language_t ToHarfbuzzLanguage(int8_t lang)
  {
    auto const resolve = [](int8_t code) -> hb_language_t
    {
      auto const svLang = StringUtf8Multilang::GetLangByCode(code);
      auto const hb = hb_language_from_string(svLang.data(), static_cast<int>(svLang.size()));
      return hb == HB_LANGUAGE_INVALID ? hb_language_get_default() : hb;
    };

    // Out-of-range codes (e.g. kUnsupportedLanguageCode == -1) bypass the cache.
    if (lang < 0 || static_cast<size_t>(lang) >= m_languageCache.size())
      return resolve(lang);

    auto const idx = static_cast<size_t>(lang);
    if (auto const cached = m_languageCache[idx]; cached != HB_LANGUAGE_INVALID)
      return cached;

    auto const hb = resolve(lang);
    m_languageCache[idx] = hb;
    return hb;
  }

  FT_Library m_library = nullptr;
  TUniBlocks m_blocks;
  TUniBlockIter m_lastUsedBlock;
  std::vector<std::unique_ptr<Font>> m_fonts;

  TextMetricsCache m_textMetricsCache;
  // Codepoints already reported as having no font — avoids per-character LOG spam on every render.
  std::unordered_set<strings::UniChar> m_loggedMissingChars;
  hb_buffer_t * m_harfbuzzBuffer = nullptr;
  std::array<hb_language_t, StringUtf8Multilang::kMaxSupportedLanguages> m_languageCache{};
};

// Destructor is defined where pimpl's destructor is already known.
GlyphManager::~GlyphManager() = default;

GlyphManager::GlyphManager(Params const & params) : m_impl(std::make_unique<Impl>())
{
  using TFontAndBlockName = std::pair<std::string, std::string>;
  using TFontLst = buffer_vector<TFontAndBlockName, 64>;

  TFontLst whitelst;
  TFontLst blacklst;

  m_impl->m_blocks.reserve(160);
  ParseUniBlocks(params.m_uniBlocks, [this](std::string const & name, strings::UniChar start, strings::UniChar end)
  { m_impl->m_blocks.emplace_back(name, start, end); });

  ParseFontList(params.m_whitelist, [&whitelst](std::string const & ubName, std::string const & fontName)
  { whitelst.emplace_back(fontName, ubName); });

  ParseFontList(params.m_blacklist, [&blacklst](std::string const & ubName, std::string const & fontName)
  { blacklst.emplace_back(fontName, ubName); });

  m_impl->m_fonts.reserve(params.m_fonts.size());

  CHECK_EQUAL(FREETYPE_CHECK(FT_Init_FreeType(&m_impl->m_library)), 0, ());

  // Default Freetype spread/sdf border is 8.
  static constexpr FT_Int kSdfBorder = dp::kSdfBorder;
  for (auto const module : {"sdf", "bsdf"})
    FREETYPE_CHECK(FT_Property_Set(m_impl->m_library, module, "spread", &kSdfBorder));

  // Resolve once: only CJK collection fonts use this.
  languages::CJKResolver const cjk;

  for (auto const & fontName : params.m_fonts)
  {
    if (std::ranges::any_of(blacklst, [&fontName](TFontAndBlockName const & p)
    { return p.first == fontName && p.second == kAllBlocks; }))
      continue;

    std::vector<FT_ULong> charCodes;
    try
    {
      FT_Long const faceIndex = languages::CJKResolver::IsCJKContainerFileName(fontName)
                                  ? PickCJKFaceIndex(fontName, m_impl->m_library, cjk.User())
                                  : 0;
      m_impl->m_fonts.emplace_back(
          std::make_unique<Font>(GetPlatform().GetReader(fontName), m_impl->m_library, faceIndex));
      m_impl->m_fonts.back()->GetCharcodes(charCodes);
    }
    catch (RootException const & e)
    {
      LOG(LWARNING, ("Error reading font file =", fontName, "; Reason =", e.what()));
      continue;
    }

    using BlockIndex = size_t;
    using CharCounter = int;
    using CoverNode = std::pair<BlockIndex, CharCounter>;
    using CoverInfo = std::vector<CoverNode>;

    size_t currentUniBlock = 0;
    CoverInfo coverInfo;
    for (auto const charCode : charCodes)
    {
      size_t block = currentUniBlock;
      while (block < m_impl->m_blocks.size())
      {
        if (m_impl->m_blocks[block].HasSymbol(static_cast<strings::UniChar>(charCode)))
          break;
        ++block;
      }

      if (block < m_impl->m_blocks.size())
      {
        if (coverInfo.empty() || coverInfo.back().first != block)
          coverInfo.emplace_back(block, 1);
        else
          ++coverInfo.back().second;

        currentUniBlock = block;
      }
    }

    auto const enumerateFn = [this, &coverInfo, &fontName](TFontLst const & lst, auto && fn)
    {
      for (auto const & b : lst)
      {
        if (b.first != fontName)
          continue;

        for (CoverNode & node : coverInfo)
        {
          auto const & uniBlock = m_impl->m_blocks[node.first];
          if (uniBlock.m_name == b.second)
          {
            fn(uniBlock, node);
            break;
          }
          else if (b.second == kAllBlocks)
          {
            fn(uniBlock, node);
          }
        }
      }
    };

    enumerateFn(blacklst, [](UnicodeBlock const &, CoverNode & node) { node.second = 0; });

    enumerateFn(whitelst, [this](UnicodeBlock const & uniBlock, CoverNode & node)
    { node.second = static_cast<int>(uniBlock.m_end + 1 - uniBlock.m_start + m_impl->m_fonts.size()); });

    for (CoverNode const & node : coverInfo)
    {
      UnicodeBlock & uniBlock = m_impl->m_blocks[node.first];
      uniBlock.m_fontsWeight.resize(m_impl->m_fonts.size(), 0);
      uniBlock.m_fontsWeight.back() = node.second;
    }
  }

  m_impl->m_lastUsedBlock = m_impl->m_blocks.end();

  LOG(LDEBUG, ("How unicode blocks are mapped on font files:"));

  // Loaded font count is bounded above by the requested count: blacklist matches and font-read
  // failures both skip without inserting into m_fonts.
  ASSERT_LESS_OR_EQUAL(m_impl->m_fonts.size(), params.m_fonts.size(), ());

  for (auto const & b : m_impl->m_blocks)
  {
    auto const & weights = b.m_fontsWeight;
    ASSERT_LESS_OR_EQUAL(weights.size(), m_impl->m_fonts.size(), ());
    if (weights.empty())
    {
      LOG_SHORT(LDEBUG, (b.m_name, "is unsupported"));
    }
    else
    {
      size_t const ind = std::ranges::max_element(weights) - weights.begin();
      LOG_SHORT(LDEBUG, (b.m_name, "is in", params.m_fonts[ind]));
    }
  }
}

namespace
{
// Locates the unicode block containing `unicodePoint`, or returns end() if none does. The blocks
// are sorted by end-codepoint, so a lower_bound + HasSymbol check finds the unique candidate.
TUniBlockIter FindBlockForCodepoint(TUniBlocks const & blocks, strings::UniChar unicodePoint)
{
  auto const iter = std::lower_bound(blocks.cbegin(), blocks.cend(), unicodePoint,
                                     [](UnicodeBlock const & block, strings::UniChar v) { return block.m_end < v; });
  if (iter == blocks.cend() || !iter->HasSymbol(unicodePoint))
    return blocks.cend();
  return iter;
}
}  // namespace

int GlyphManager::GetFontIndex(strings::UniChar unicodePoint)
{
  // Last-used block is a single-entry MRU cache: most map labels hit the same block (Latin, CJK,
  // etc.) repeatedly within a single ShapeText, so the cached pointer skips the lower_bound walk.
  if (m_impl->m_lastUsedBlock != m_impl->m_blocks.cend() && m_impl->m_lastUsedBlock->HasSymbol(unicodePoint))
    return FindFontIndexInBlock(*m_impl->m_lastUsedBlock, unicodePoint);

  auto const iter = FindBlockForCodepoint(m_impl->m_blocks, unicodePoint);
  if (iter == m_impl->m_blocks.cend())
    return kInvalidFont;

  m_impl->m_lastUsedBlock = iter;
  return FindFontIndexInBlock(*iter, unicodePoint);
}

int GlyphManager::FindFontIndexInBlock(UnicodeBlock const & block, strings::UniChar unicodePoint) const
{
  ASSERT(block.HasSymbol(unicodePoint), ());
  for (int fontIndex = block.GetFontOffset(kInvalidFont); fontIndex != kInvalidFont;
       fontIndex = block.GetFontOffset(fontIndex))
  {
    ASSERT_LESS(fontIndex, static_cast<int>(m_impl->m_fonts.size()), ());
    auto const & f = m_impl->m_fonts[fontIndex];
    if (f->HasGlyph(unicodePoint))
      return fontIndex;
  }

  return kInvalidFont;
}

void GlyphManager::MarkGlyphReady(GlyphFontAndId key)
{
  ASSERT_GREATER_OR_EQUAL(key.m_fontIndex, 0, ());
  ASSERT_LESS(key.m_fontIndex, static_cast<int>(m_impl->m_fonts.size()), ());
  m_impl->m_fonts[key.m_fontIndex]->MarkGlyphReady(key.m_glyphId);
}

bool GlyphManager::AreGlyphsReady(TGlyphs const & glyphs) const
{
  for (auto [fontIndex, glyphId] : glyphs)
  {
    ASSERT_NOT_EQUAL(fontIndex, kInvalidFont, ());

    if (!m_impl->m_fonts[fontIndex]->IsGlyphReady(glyphId))
      return false;
  }

  return true;
}

// TODO(AB): Check and support invalid glyphs.
GlyphImage GlyphManager::GetGlyphImage(GlyphFontAndId key, bool sdf)
{
  return m_impl->m_fonts[key.m_fontIndex]->GetGlyphImage(key.m_glyphId, sdf);
}

// This method is NOT multithreading-safe.
text::TextMetrics GlyphManager::ShapeText(std::string_view utf8, int8_t lang)
{
  // The cache greatly speeds up text metrics calculation; observed 80+% hit ratio.
  TextMetricsCacheKeyView const cacheKey{utf8, lang};
  if (auto const * cached = m_impl->m_textMetricsCache.Find(cacheKey))
    return *cached;

  auto const [text, segments] = harfbuzz_shaping::GetTextSegments(utf8);

  hb_language_t const hbLanguage = m_impl->ToHarfbuzzLanguage(lang);

  text::TextMetrics allGlyphs;
  // For SplitText it's enough to know if the last visual (first logical) segment is RTL.
  allGlyphs.m_isRTL = segments.back().m_direction == HB_DIRECTION_RTL;

  // text.size() (UTF-16 unit count) is an upper bound on the glyph count for most scripts:
  // glyph count <= codepoint count <= UTF-16 unit count. Slight over-allocation for emoji-heavy
  // text (surrogate pairs) is preferable to walking the buffer with icu countChar32 just to size
  // a reserve hint.
  allGlyphs.m_glyphs.reserve(text.size());

  for (auto const & substring : segments)
  {
    // RTL run reversal below assumes each segment is uni-directional. GetSingleTextLineRuns guarantees this today;
    // if the contract changes (mixed embedding levels or HB_DIRECTION_INVALID get bundled), reversing would corrupt
    // glyph order silently.
    ASSERT(substring.m_direction == HB_DIRECTION_LTR || substring.m_direction == HB_DIRECTION_RTL,
           ("ShapeText assumes per-segment uni-directional runs", substring.m_direction));

    buffer_vector<FontRun, 4> fontRuns;
    int currentFontIndex = kInvalidFont;
    int32_t runStart = substring.m_start;
    int32_t runEnd = substring.m_start;

    auto characterIter = text.begin() + substring.m_start;
    auto const end = characterIter + substring.m_length;
    while (characterIter != end)
    {
      auto const characterStart = static_cast<int32_t>(characterIter - text.begin());
      auto const u32Character = utf8::unchecked::next16(characterIter);
      auto const characterEnd = static_cast<int32_t>(characterIter - text.begin());

      // GetFontIndex's MRU block cache (m_lastUsedBlock) handles the common case where
      // adjacent characters share a unicode block, so this per-character call is cheap.
      int const fontIndex = GetFontIndex(u32Character);
      if (fontIndex < 0)
      {
        if (m_impl->m_loggedMissingChars.insert(u32Character).second)
          LOG(LWARNING, ("No font for codepoint", NumToHex(u32Character)));
        // Keep the codepoint in the pending/current run. HB will emit glyph id 0
        // (.notdef) in that run's font instead of silently dropping the character.
        runEnd = characterEnd;
        continue;
      }

      if (currentFontIndex == kInvalidFont)
      {
        // Preserve any pending no-font prefix so it inherits this first usable font.
        currentFontIndex = fontIndex;
      }
      else if (fontIndex != currentFontIndex)
      {
        fontRuns.push_back({runStart, runEnd - runStart, currentFontIndex});
        runStart = characterStart;
        currentFontIndex = fontIndex;
      }

      runEnd = characterEnd;
    }

    if (currentFontIndex != kInvalidFont)
      fontRuns.push_back({runStart, runEnd - runStart, currentFontIndex});
    else if (runEnd > runStart && !m_impl->m_fonts.empty())
    {
      // No supported codepoint was found. Shape the whole segment with a
      // deterministic font so unsupported visible characters still emit .notdef.
      fontRuns.push_back({runStart, runEnd - runStart, 0});
    }

    auto const shapeFontRun = [this, &text, hbLanguage, &substring, &allGlyphs](FontRun const & run)
    {
      hb_buffer_clear_contents(m_impl->m_harfbuzzBuffer);
      hb_buffer_add_utf16(m_impl->m_harfbuzzBuffer, reinterpret_cast<uint16_t const *>(text.data()),
                          static_cast<int>(text.size()), run.m_start, run.m_length);
      hb_buffer_set_direction(m_impl->m_harfbuzzBuffer, substring.m_direction);
      hb_buffer_set_script(m_impl->m_harfbuzzBuffer, substring.m_script);
      hb_buffer_set_language(m_impl->m_harfbuzzBuffer, hbLanguage);

      m_impl->m_fonts[run.m_fontIndex]->Shape(m_impl->m_harfbuzzBuffer, run.m_fontIndex, allGlyphs);
    };

    if (substring.m_direction == HB_DIRECTION_RTL)
      for (size_t i = fontRuns.size(); i > 0; --i)
        shapeFontRun(fontRuns[i - 1]);
    else
      for (auto const & run : fontRuns)
        shapeFontRun(run);
  }

  // Uncomment utf8 printing for debugging if necessary. It crashes JNI with non-modified UTF-8 strings on Android 5
  // and 6. See https://github.com/organicmaps/organicmaps/issues/10685
  if (allGlyphs.m_glyphs.empty())
    LOG(LWARNING, ("No glyphs were found in all fonts for string with characters in warnings above" /*, utf8*/));

  return m_impl->m_textMetricsCache.Insert(TextMetricsCacheKey{std::string(utf8), lang}, std::move(allGlyphs));
}

text::TextMetrics GlyphManager::ShapeText(std::string_view utf8, char const * lang)
{
  return ShapeText(utf8, StringUtf8Multilang::GetLangIndex(lang));
}

}  // namespace dp

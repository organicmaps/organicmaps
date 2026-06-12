#include "indexer/drules_format.hpp"

#include "coding/reader.hpp"
#include "coding/varint.hpp"

#include "base/assert.hpp"

#include <bit>
#include <cstring>
#include <utility>

namespace drule
{
namespace
{
using Source = ReaderSource<MemReaderWithExceptions>;

class Decoder
{
public:
  explicit Decoder(std::string_view blob) : m_reader(blob), m_src(m_reader) {}

  // Returns false on magic/version mismatch (caller falls back to the bundled file). A corrupt body
  // trips a DEBUG-only ASSERT; release trusts the file (a truncated read still throws via the Reader).
  bool Decode(DrulesFormat & out)
  {
    // Too short to even hold magic + version: treat as a mismatch (e.g. a truncated override file),
    // so the caller falls back to the bundled file instead of getting a thrown read exception.
    if (m_src.Size() < sizeof(kFormatMagic) + 1)
      return false;

    char magic[sizeof(kFormatMagic)];
    m_src.Read(magic, sizeof(magic));
    if (std::memcmp(magic, kFormatMagic, sizeof(magic)) != 0)
      return false;
    if (ReadU8() != kFormatVersion)
      return false;

    uint32_t const variantCount = ReadU8();
    out.variants.resize(variantCount);
    for (auto & v : out.variants)
      v = ReadRawString();

    ReadStringTable();
    ReadColorTable(out, variantCount);
    ReadNamedColors(out);
    ReadRuleCounts(out);
    ReadTypes(out);
    ReadOverrides(out, variantCount);
    return true;
  }

private:
  // Reads a string that is stored inline (varuint length + UTF-8), not via the string table.
  std::string ReadRawString()
  {
    uint32_t const len = ReadCount();
    std::string s;
    s.resize_and_overwrite(len, [this](char * p, size_t n)
    {
      if (n > 0)
        m_src.Read(p, n);
      return n;
    });
    return s;
  }

  uint8_t ReadU8() { return ReadPrimitiveFromSource<uint8_t>(m_src); }
  uint32_t ReadVarU() { return ReadVarUint<uint32_t>(m_src); }
  int32_t ReadVarI() { return ReadVarInt<int32_t>(m_src); }
  float ReadF32() { return std::bit_cast<float>(ReadPrimitiveFromSource<uint32_t>(m_src)); }

  // Rejects a length/count/size that can't fit in the bytes that remain (every counted element or
  // string byte costs at least one byte). Corrupt drules are a build/designer error, never a runtime
  // condition for the bundled file, so this is a DEBUG-only ASSERT: release trusts the file and skips
  // the check for speed (a truly corrupt override still surfaces via a Reader read-past-end there).
  void CheckRemaining([[maybe_unused]] uint64_t n, [[maybe_unused]] char const * what) const
  {
    ASSERT_LESS_OR_EQUAL(n, m_src.Size(), (what, "exceeds remaining drules bytes"));
  }

  // Reads a varuint length/count that drives an allocation or a read loop, bounded by CheckRemaining.
  uint32_t ReadCount()
  {
    uint32_t const n = ReadVarU();
    CheckRemaining(n, "Drules length/count");
    return n;
  }

  std::string const & Str(uint32_t idx) const
  {
    ASSERT_LESS(idx, m_strings.size(), ("Bad drules string index"));
    return m_strings[idx];
  }

  // Reads a color-palette index, validated against the color table (read in ReadColorTable) with a
  // DEBUG-only ASSERT. A bad index means a broken build/designer file, caught in debug and tests;
  // release trusts the file and skips the check (an out-of-range index would otherwise OOB-read
  // colors[variant][idx] later in RulesHolder::LoadFromFormat).
  uint32_t ReadColorIdx()
  {
    uint32_t const idx = ReadVarU();
    ASSERT_LESS(idx, m_colorCount, ("Bad drules color index"));
    return idx;
  }

  // Each section is prefixed with its varuint byte size to allow forward-skipping. Read the size
  // first: its own bytes must be consumed before Pos() marks the start of the section body.
  uint64_t BeginSection()
  {
    uint64_t const size = ReadVarUint<uint64_t>(m_src);
    CheckRemaining(size, "Drules section size");
    return m_src.Pos() + size;
  }
  void EndSection(uint64_t end)
  {
    uint64_t const pos = m_src.Pos();
    ASSERT_LESS_OR_EQUAL(pos, end, ("Drules section overrun"));
    m_src.Skip(end - pos);
  }

  void ReadStringTable()
  {
    uint64_t const end = BeginSection();
    m_strings.resize(ReadCount());
    for (auto & s : m_strings)
      s = ReadRawString();
    ASSERT(!m_strings.empty() && m_strings.front().empty(), ("Drules string table must start with the empty string"));
    EndSection(end);
  }

  void ReadColorTable(DrulesFormat & out, uint32_t variantCount)
  {
    uint64_t const end = BeginSection();
    m_colorCount = ReadCount();
    out.colors.resize(variantCount);
    for (auto & palette : out.colors)
    {
      palette.resize(m_colorCount);
      for (auto & c : palette)
        c = ReadPrimitiveFromSource<uint32_t>(m_src);
    }
    EndSection(end);
  }

  void ReadNamedColors(DrulesFormat & out)
  {
    uint64_t const end = BeginSection();
    out.namedColors.resize(ReadCount());
    for (auto & nc : out.namedColors)
    {
      nc.name = Str(ReadVarU());
      nc.colorIdx = ReadColorIdx();
    }
    EndSection(end);
  }

  void ReadRuleCounts(DrulesFormat & out)
  {
    uint64_t const end = BeginSection();
    for (auto & rc : out.ruleCounts)
      rc = ReadVarU();
    EndSection(end);
  }

  void ReadTypes(DrulesFormat & out)
  {
    uint64_t const end = BeginSection();
    out.types.resize(ReadCount());
    for (auto & t : out.types)
    {
      t.name = Str(ReadVarU());
      t.elements.resize(ReadCount());
      for (auto & e : t.elements)
        e = ReadElement();
    }
    EndSection(end);
  }

  void ReadOverrides(DrulesFormat & out, uint32_t variantCount)
  {
    uint64_t const end = BeginSection();
    out.overrides.resize(variantCount);
    for (auto & variantOverrides : out.overrides)
    {
      variantOverrides.resize(ReadCount());
      for (auto & o : variantOverrides)
      {
        o.typeIdx = ReadVarU();
        o.elemIdx = ReadVarU();
        o.element = ReadElement();
      }
    }
    EndSection(end);
  }

  Element ReadElement()
  {
    Element e;
    e.scale = ReadU8();
    e.applyIf.resize(ReadCount());
    for (auto & a : e.applyIf)
      a = Str(ReadVarU());

    // Payloads follow in kindMask bit order: lines, area, symbol, caption, path_text, shield.
    uint8_t const kind = ReadU8();
    if (kind & kKindLines)
    {
      e.lines.resize(ReadCount());
      for (auto & l : e.lines)
        l = ReadLineRule();
    }
    if (kind & kKindArea)
      e.area = ReadAreaRule();
    if (kind & kKindSymbol)
      e.symbol = ReadSymbolRule();
    if (kind & kKindCaption)
      e.caption = ReadCaptionRule();
    if (kind & kKindPathText)
      e.pathText = ReadCaptionRule();
    if (kind & kKindShield)
      e.shield = ReadShieldRule();
    return e;
  }

  // Reads the dashdot/pathsym tail shared by LineRule and LineDef.
  void ReadLineExtras(uint8_t flags, LineDef & d)
  {
    if (flags & kLineFlagDashDot)
    {
      DashDot dd;
      uint32_t const n = ReadCount();
      dd.dd.reserve(n);
      for (uint32_t i = 0; i < n; ++i)
        dd.dd.push_back(ReadF32());
      dd.offset = ReadF32();
      d.dashdot = std::move(dd);
    }
    if (flags & kLineFlagPathSym)
    {
      PathSym ps;
      ps.name = Str(ReadVarU());
      ps.step = ReadF32();
      ps.offset = ReadF32();
      d.pathsym = std::move(ps);
    }
  }

  LineRule ReadLineRule()
  {
    LineRule r;
    uint8_t const flags = ReadU8();
    r.width = ReadF32();
    r.color = ReadColorIdx();
    r.priority = ReadVarI();
    r.join = static_cast<LineJoin>(ReadU8());
    r.cap = static_cast<LineCap>(ReadU8());
    ReadLineExtras(flags, r);
    return r;
  }

  LineDef ReadLineDef()
  {
    LineDef d;
    uint8_t const flags = ReadU8();
    d.width = ReadF32();
    d.color = ReadColorIdx();
    d.join = static_cast<LineJoin>(ReadU8());
    d.cap = static_cast<LineCap>(ReadU8());
    ReadLineExtras(flags, d);
    return d;
  }

  AreaRule ReadAreaRule()
  {
    AreaRule a;
    uint8_t const flags = ReadU8();
    a.color = ReadColorIdx();
    a.priority = ReadVarI();
    if (flags & kAreaFlagBorder)
      a.border = ReadLineDef();
    return a;
  }

  SymbolRule ReadSymbolRule()
  {
    SymbolRule s;
    s.name = Str(ReadVarU());
    s.apply_for_type = ReadVarI();
    s.priority = ReadVarI();
    s.min_distance = ReadVarI();
    return s;
  }

  CaptionDef ReadCaptionDef()
  {
    CaptionDef c;
    c.height = ReadVarI();
    c.color = ReadColorIdx();
    c.stroke_color = ReadColorIdx();
    c.offset_x = ReadVarI();
    c.offset_y = ReadVarI();
    c.text = Str(ReadVarU());
    c.is_optional = ReadU8() != 0;
    return c;
  }

  CaptionRule ReadCaptionRule()
  {
    CaptionRule c;
    uint8_t const flags = ReadU8();
    if (flags & kCaptionFlagPrimary)
      c.primary = ReadCaptionDef();
    if (flags & kCaptionFlagSecondary)
      c.secondary = ReadCaptionDef();
    c.priority = ReadVarI();
    return c;
  }

  ShieldRule ReadShieldRule()
  {
    ShieldRule s;
    s.height = ReadVarI();
    s.color = ReadColorIdx();
    s.stroke_color = ReadColorIdx();
    s.priority = ReadVarI();
    s.min_distance = ReadVarI();
    s.text_color = ReadColorIdx();
    s.text_stroke_color = ReadColorIdx();
    return s;
  }

  MemReaderWithExceptions m_reader;
  Source m_src;
  std::vector<std::string> m_strings;
  uint32_t m_colorCount = 0;
};
}  // namespace

bool DecodeDrules(std::string_view blob, DrulesFormat & out)
{
  return Decoder(blob).Decode(out);
}
}  // namespace drule

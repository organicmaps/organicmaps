#include "testing/testing.hpp"

#include "indexer/drules_format.hpp"

#include "coding/varint.hpp"
#include "coding/write_to_sink.hpp"
#include "coding/writer.hpp"

#include "base/assert.hpp"
#include "base/exception.hpp"

#include <bit>
#include <cstdint>
#include <map>
#include <string>
#include <utility>
#include <vector>

namespace drules_format_tests
{
namespace
{
using namespace drule;

#ifdef DEBUG
// The decoder validates corrupt input with DEBUG-only ASSERTs (release compiles them out and trusts
// the bundled file). Route a fired assert to a flag so the tests below can confirm corruption is
// caught in debug; returning false lets decoding unwind through its normal path instead of crashing.
bool g_assertRaised = false;
bool OnDrulesAssert(base::SrcPoint const &, std::string const &)
{
  g_assertRaised = true;
  return false;
}
#endif

using Buffer = std::vector<uint8_t>;
using Sink = MemWriter<Buffer>;

void PutU8(Sink & s, uint8_t v)
{
  WriteToSink(s, v);
}
void PutU32(Sink & s, uint32_t v)
{
  WriteToSink(s, v);
}
void PutVarU(Sink & s, uint32_t v)
{
  WriteVarUint(s, v);
}
void PutVarI(Sink & s, int32_t v)
{
  WriteVarInt(s, v);
}
void PutF32(Sink & s, float v)
{
  WriteToSink(s, std::bit_cast<uint32_t>(v));
}

void PutRawStr(Sink & s, std::string const & str)
{
  WriteVarUint(s, static_cast<uint32_t>(str.size()));
  if (!str.empty())
    s.Write(str.data(), str.size());
}

// Mirror of the drules_format.cpp decoder used only to build synthetic blobs for the round-trip
// test. Kept symmetric with the decoder method-by-method so a layout change must touch both.
class Encoder
{
public:
  std::string Encode(DrulesFormat const & f)
  {
    // Build the sections that reference strings first, so the string table is complete by the
    // time it is serialized (it must precede them in the stream).
    Buffer const namedColors = EncodeNamedColors(f);
    Buffer const types = EncodeTypes(f);
    Buffer const overrides = EncodeOverrides(f);
    Buffer const stringTable = EncodeStringTable();
    Buffer const colorTable = EncodeColorTable(f);
    Buffer const ruleCounts = EncodeRuleCounts(f);

    Buffer out;
    Sink sink(out);
    sink.Write(kFormatMagic, sizeof(kFormatMagic));
    PutU8(sink, kFormatVersion);
    PutU8(sink, static_cast<uint8_t>(f.variants.size()));
    for (auto const & v : f.variants)
      PutRawStr(sink, v);
    WriteSection(sink, stringTable);
    WriteSection(sink, colorTable);
    WriteSection(sink, namedColors);
    WriteSection(sink, ruleCounts);
    WriteSection(sink, types);
    WriteSection(sink, overrides);
    return std::string(out.begin(), out.end());
  }

private:
  static void WriteSection(Sink & sink, Buffer const & body)
  {
    WriteVarUint(sink, static_cast<uint64_t>(body.size()));
    if (!body.empty())
      sink.Write(body.data(), body.size());
  }

  uint32_t Intern(std::string const & s)
  {
    auto const [it, inserted] = m_strIdx.emplace(s, static_cast<uint32_t>(m_strings.size()));
    if (inserted)
      m_strings.push_back(s);
    return it->second;
  }

  Buffer EncodeStringTable()
  {
    Buffer b;
    Sink s(b);
    PutVarU(s, static_cast<uint32_t>(m_strings.size()));
    for (auto const & str : m_strings)
      PutRawStr(s, str);
    return b;
  }

  Buffer EncodeColorTable(DrulesFormat const & f)
  {
    Buffer b;
    Sink s(b);
    PutVarU(s, f.colors.empty() ? 0 : static_cast<uint32_t>(f.colors.front().size()));
    for (auto const & palette : f.colors)
      for (uint32_t c : palette)
        PutU32(s, c);
    return b;
  }

  Buffer EncodeNamedColors(DrulesFormat const & f)
  {
    Buffer b;
    Sink s(b);
    PutVarU(s, static_cast<uint32_t>(f.namedColors.size()));
    for (auto const & nc : f.namedColors)
    {
      PutVarU(s, Intern(nc.name));
      PutVarU(s, nc.colorIdx);
    }
    return b;
  }

  Buffer EncodeRuleCounts(DrulesFormat const & f)
  {
    Buffer b;
    Sink s(b);
    for (uint32_t rc : f.ruleCounts)
      PutVarU(s, rc);
    return b;
  }

  Buffer EncodeTypes(DrulesFormat const & f)
  {
    Buffer b;
    Sink s(b);
    PutVarU(s, static_cast<uint32_t>(f.types.size()));
    for (auto const & t : f.types)
    {
      PutVarU(s, Intern(t.name));
      PutVarU(s, static_cast<uint32_t>(t.elements.size()));
      for (auto const & e : t.elements)
        WriteElement(s, e);
    }
    return b;
  }

  Buffer EncodeOverrides(DrulesFormat const & f)
  {
    Buffer b;
    Sink s(b);
    for (auto const & variantOverrides : f.overrides)
    {
      PutVarU(s, static_cast<uint32_t>(variantOverrides.size()));
      for (auto const & o : variantOverrides)
      {
        PutVarU(s, o.typeIdx);
        PutVarU(s, o.elemIdx);
        WriteElement(s, o.element);
      }
    }
    return b;
  }

  void WriteElement(Sink & s, Element const & e)
  {
    PutU8(s, e.scale);
    PutVarU(s, static_cast<uint32_t>(e.applyIf.size()));
    for (auto const & a : e.applyIf)
      PutVarU(s, Intern(a));

    uint8_t kind = 0;
    if (!e.lines.empty())
      kind |= kKindLines;
    if (e.area)
      kind |= kKindArea;
    if (e.symbol)
      kind |= kKindSymbol;
    if (e.caption)
      kind |= kKindCaption;
    if (e.pathText)
      kind |= kKindPathText;
    if (e.shield)
      kind |= kKindShield;
    PutU8(s, kind);

    if (!e.lines.empty())
    {
      PutVarU(s, static_cast<uint32_t>(e.lines.size()));
      for (auto const & l : e.lines)
        WriteLineRule(s, l);
    }
    if (e.area)
      WriteAreaRule(s, *e.area);
    if (e.symbol)
      WriteSymbolRule(s, *e.symbol);
    if (e.caption)
      WriteCaptionRule(s, *e.caption);
    if (e.pathText)
      WriteCaptionRule(s, *e.pathText);
    if (e.shield)
      WriteShieldRule(s, *e.shield);
  }

  static uint8_t LineFlags(LineDef const & d)
  {
    uint8_t f = 0;
    if (d.dashdot)
      f |= kLineFlagDashDot;
    if (d.pathsym)
      f |= kLineFlagPathSym;
    return f;
  }

  void WriteLineExtras(Sink & s, LineDef const & d)
  {
    if (d.dashdot)
    {
      PutVarU(s, static_cast<uint32_t>(d.dashdot->dd.size()));
      for (float v : d.dashdot->dd)
        PutF32(s, v);
      PutF32(s, d.dashdot->offset);
    }
    if (d.pathsym)
    {
      PutVarU(s, Intern(d.pathsym->name));
      PutF32(s, d.pathsym->step);
      PutF32(s, d.pathsym->offset);
    }
  }

  void WriteLineRule(Sink & s, LineRule const & r)
  {
    PutU8(s, LineFlags(r));
    PutF32(s, r.width);
    PutVarU(s, r.color);
    PutVarI(s, r.priority);
    PutU8(s, static_cast<uint8_t>(r.join));
    PutU8(s, static_cast<uint8_t>(r.cap));
    WriteLineExtras(s, r);
  }

  void WriteLineDef(Sink & s, LineDef const & d)
  {
    PutU8(s, LineFlags(d));
    PutF32(s, d.width);
    PutVarU(s, d.color);
    PutU8(s, static_cast<uint8_t>(d.join));
    PutU8(s, static_cast<uint8_t>(d.cap));
    WriteLineExtras(s, d);
  }

  void WriteAreaRule(Sink & s, AreaRule const & a)
  {
    PutU8(s, a.border ? kAreaFlagBorder : 0);
    PutVarU(s, a.color);
    PutVarI(s, a.priority);
    if (a.border)
      WriteLineDef(s, *a.border);
  }

  void WriteSymbolRule(Sink & s, SymbolRule const & sym)
  {
    PutVarU(s, Intern(sym.name));
    PutVarI(s, sym.apply_for_type);
    PutVarI(s, sym.priority);
    PutVarI(s, sym.min_distance);
  }

  void WriteCaptionDef(Sink & s, CaptionDef const & c)
  {
    PutVarI(s, c.height);
    PutVarU(s, c.color);
    PutVarU(s, c.stroke_color);
    PutVarI(s, c.offset_x);
    PutVarI(s, c.offset_y);
    PutVarU(s, Intern(c.text));
    PutU8(s, c.is_optional ? 1 : 0);
  }

  void WriteCaptionRule(Sink & s, CaptionRule const & c)
  {
    uint8_t flags = 0;
    if (c.primary)
      flags |= kCaptionFlagPrimary;
    if (c.secondary)
      flags |= kCaptionFlagSecondary;
    PutU8(s, flags);
    if (c.primary)
      WriteCaptionDef(s, *c.primary);
    if (c.secondary)
      WriteCaptionDef(s, *c.secondary);
    PutVarI(s, c.priority);
  }

  void WriteShieldRule(Sink & s, ShieldRule const & sh)
  {
    PutVarI(s, sh.height);
    PutVarU(s, sh.color);
    PutVarU(s, sh.stroke_color);
    PutVarI(s, sh.priority);
    PutVarI(s, sh.min_distance);
    PutVarU(s, sh.text_color);
    PutVarU(s, sh.text_stroke_color);
  }

  std::vector<std::string> m_strings{""};
  std::map<std::string, uint32_t> m_strIdx{{"", 0}};
};

// A fixture exercising every section, every rule kind, both optional/dashdot/pathsym/border
// branches, two variants and a non-empty override. Distinct values per field catch field-order
// or signed/unsigned mistakes in the decoder.
DrulesFormat BuildFixture()
{
  DrulesFormat f;
  f.variants = {"light", "dark"};
  f.colors = {
      {0xFFFFFF, 0xAABBCC, 0x112233, 0x445566},  // light palette
      {0x1C1C1C, 0x010203, 0x0A0B0C, 0x0D0E0F},  // dark palette
  };
  f.namedColors = {{"GuiText", 3}, {"Bg", 0}};
  // Distinct values so a wrong RuleKind ordering in the decoder can't round-trip unnoticed.
  f.ruleCounts = {3, 5, 7, 11, 13, 17};

  {
    TypeEntry t;
    t.name = "natural-land";
    Element e;
    e.scale = 0;
    AreaRule a;
    a.color = 1;
    a.priority = -2000;
    e.area = a;
    t.elements.push_back(std::move(e));
    f.types.push_back(std::move(t));
  }

  {
    TypeEntry t;
    t.name = "highway-residential";

    {
      Element e;
      e.scale = 12;
      e.applyIf = {"population>=1000", "z<14"};

      // l1 carries BOTH dashdot and pathsym to pin their on-disk interleave order.
      LineRule l1;
      l1.width = 1.3f;
      l1.color = 0;
      l1.priority = 180;
      l1.join = LineJoin::Round;
      l1.cap = LineCap::Butt;
      DashDot dd;
      dd.dd = {4.5f, 3.0f};
      dd.offset = 0.5f;
      l1.dashdot = dd;
      PathSym ps1;
      ps1.name = "arrow";
      ps1.step = 7.0f;
      ps1.offset = -1.5f;  // negative float
      l1.pathsym = ps1;
      e.lines.push_back(l1);

      LineRule l2;
      l2.width = 2.0f;
      l2.color = 2;
      l2.priority = 100;
      l2.join = LineJoin::Bevel;
      l2.cap = LineCap::Square;
      PathSym ps2;
      ps2.name = "arrow";  // reused string => exercises the encoder's interning dedup
      ps2.step = 10.0f;
      ps2.offset = 2.0f;
      l2.pathsym = ps2;
      e.lines.push_back(l2);

      CaptionRule cap;
      CaptionDef primary;
      primary.height = 11;
      primary.color = 3;
      primary.stroke_color = 1;
      primary.offset_x = -3;  // negative
      primary.offset_y = 2;
      primary.text = "name";
      cap.primary = primary;
      CaptionDef secondary;
      secondary.height = 9;
      secondary.color = 1;
      secondary.stroke_color = 2;
      secondary.offset_x = 1;
      secondary.offset_y = -1;  // negative
      secondary.text = "ref";
      secondary.is_optional = true;
      cap.secondary = secondary;
      cap.priority = 4100;
      e.caption = cap;

      ShieldRule sh;
      sh.height = 10;
      sh.color = 0;
      sh.stroke_color = 1;
      sh.priority = 4000;
      sh.min_distance = 50;
      sh.text_color = 2;
      sh.text_stroke_color = 3;
      e.shield = sh;

      t.elements.push_back(std::move(e));
    }

    {
      Element e;
      e.scale = 15;

      SymbolRule sym;
      sym.name = "dot";
      sym.apply_for_type = 1;
      sym.priority = 200;
      sym.min_distance = 5;
      e.symbol = sym;

      PathTextRule pt;
      CaptionDef ptPrimary;
      ptPrimary.height = 8;
      ptPrimary.color = 1;
      ptPrimary.stroke_color = 3;
      ptPrimary.offset_y = -4;  // negative
      ptPrimary.text = "name";  // reused string
      pt.primary = ptPrimary;
      pt.priority = 300;
      e.pathText = pt;

      AreaRule a;
      a.color = 0;
      a.priority = -1500;
      LineDef border;
      border.width = 0.5f;
      border.color = 2;
      a.border = border;
      e.area = a;

      t.elements.push_back(std::move(e));
    }

    f.types.push_back(std::move(t));
  }

  // Light has no overrides; dark replaces the first element of type 1.
  f.overrides.resize(2);
  {
    Override o;
    o.typeIdx = 1;
    o.elemIdx = 0;
    o.element.scale = 12;
    LineRule l;
    l.width = 9.0f;
    l.color = 1;
    l.priority = 7;
    l.join = LineJoin::No;
    l.cap = LineCap::Square;
    o.element.lines.push_back(l);
    f.overrides[1].push_back(std::move(o));
  }

  return f;
}
}  // namespace

// The Encoder above is hand-mirrored from the decoder, so this round-trip proves the decoder is
// self-consistent with a conforming writer. DrulesFormat_GoldenKothicBlob below pins agreement with
// the actual kothic Python writer using a checked-in golden blob.
UNIT_TEST(DrulesFormat_RoundTrip)
{
  DrulesFormat const expected = BuildFixture();
  std::string const blob = Encoder().Encode(expected);

  DrulesFormat decoded;
  TEST(DecodeDrules(blob, decoded), ());
  TEST(decoded == expected, ());

  // Guard against the comparison above passing on two coincidentally-equal empty objects.
  TEST_EQUAL(decoded.types.size(), 2, ());
  TEST_EQUAL(decoded.VariantCount(), 2, ());
  TEST_EQUAL(decoded.colors.at(1).at(0), 0x1C1C1Cu, ());
  TEST(decoded.overrides.at(0).empty(), ());
  TEST_EQUAL(decoded.overrides.at(1).size(), 1, ());
}

UNIT_TEST(DrulesFormat_BadMagic)
{
  std::string blob = Encoder().Encode(BuildFixture());
  blob[0] = 'X';
  DrulesFormat decoded;
  TEST(!DecodeDrules(blob, decoded), ());
}

UNIT_TEST(DrulesFormat_BadVersion)
{
  std::string blob = Encoder().Encode(BuildFixture());
  blob[sizeof(kFormatMagic)] = static_cast<char>(kFormatVersion + 1);  // version byte follows the magic
  DrulesFormat decoded;
  TEST(!DecodeDrules(blob, decoded), ());
}

UNIT_TEST(DrulesFormat_Minimal)
{
  // Degenerate but valid: a single variant, an empty palette, no named colors and no types.
  DrulesFormat expected;
  expected.variants = {"design"};
  expected.colors = {{}};        // one variant, zero colors
  expected.overrides.resize(1);  // one (empty) variant slot

  std::string const blob = Encoder().Encode(expected);
  DrulesFormat decoded;
  TEST(DecodeDrules(blob, decoded), ());
  TEST(decoded == expected, ());
  TEST_EQUAL(decoded.VariantCount(), 1, ());
  TEST(decoded.types.empty(), ());
}

UNIT_TEST(DrulesFormat_TruncatedHeaderFallsBack)
{
  // Too short to hold magic + version => reported as a mismatch (false) so the loader falls back to
  // the bundled file, rather than throwing on a stale/corrupt WritableDir/styles override.
  DrulesFormat decoded;
  TEST(!DecodeDrules(std::string_view("OMD"), decoded), ());
  TEST(!DecodeDrules(std::string_view(), decoded), ());
}

UNIT_TEST(DrulesFormat_CorruptBodyAsserts)
{
#ifdef DEBUG
  // Truncate the last section's body so its declared size exceeds the bytes left. In DEBUG the
  // decoder catches this via ASSERT; in RELEASE the check is compiled out (the bundled file is
  // trusted), so this only verifies the debug-time guard.
  std::string blob = Encoder().Encode(BuildFixture());
  blob.pop_back();

  g_assertRaised = false;
  base::AssertFailedFn const prev = base::SetAssertFunction(&OnDrulesAssert);
  try
  {
    DrulesFormat decoded;
    DecodeDrules(blob, decoded);
  }
  catch (RootException const &)
  {}  // After the assert the decoder may also unwind via a Reader read-past-end; ignore it.
  base::SetAssertFunction(prev);

  TEST(g_assertRaised, ());
#endif
}

UNIT_TEST(DrulesFormat_BadColorIndexAsserts)
{
#ifdef DEBUG
  // A structurally valid blob whose color index points past the palette is caught by a DEBUG ASSERT
  // at decode time. In RELEASE the check is compiled out and the file is trusted (an out-of-range
  // index would otherwise OOB-read colors[variant][idx] later in RulesHolder::LoadFromFormat).
  DrulesFormat f = BuildFixture();
  f.namedColors.push_back({"Bad", 4});  // palettes hold 4 colors (indices 0..3); 4 is out of range
  std::string const blob = Encoder().Encode(f);

  g_assertRaised = false;
  base::AssertFailedFn const prev = base::SetAssertFunction(&OnDrulesAssert);
  try
  {
    DrulesFormat decoded;
    DecodeDrules(blob, decoded);
  }
  catch (RootException const &)
  {}
  base::SetAssertFunction(prev);

  TEST(g_assertRaised, ());
#endif
}

// Cross-language gate: a blob emitted by tools/kothic/src/drules.py serialize_binary() for a
// one-type fixture (one line + one caption + one named color, single "light" variant) must decode
// here to the same values. This pins the kothic writer against this decoder; regenerate the bytes
// if the wire format changes (see drules_format.hpp).
UNIT_TEST(DrulesFormat_GoldenKothicBlob)
{
  static uint8_t const kBlob[] = {
      0x4F, 0x4D, 0x44, 0x52, 0x01, 0x01, 0x05, 0x6C, 0x69, 0x67, 0x68, 0x74, 0x22, 0x04, 0x00, 0x0F, 0x68, 0x69,
      0x67, 0x68, 0x77, 0x61, 0x79, 0x2D, 0x70, 0x72, 0x69, 0x6D, 0x61, 0x72, 0x79, 0x04, 0x6E, 0x61, 0x6D, 0x65,
      0x0A, 0x46, 0x6F, 0x72, 0x65, 0x67, 0x72, 0x6F, 0x75, 0x6E, 0x64, 0x11, 0x04, 0x20, 0x20, 0x20, 0x00, 0x00,
      0x00, 0xFF, 0x00, 0x03, 0x02, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0x01, 0x03, 0x00, 0x06, 0x01, 0x00,
      0x00, 0x01, 0x00, 0x00, 0x1B, 0x01, 0x01, 0x01, 0x0A, 0x00, 0x09, 0x01, 0x00, 0x00, 0x00, 0x00, 0x40, 0x01,
      0xC8, 0x01, 0x01, 0x02, 0x01, 0x18, 0x02, 0x03, 0x00, 0x00, 0x02, 0x00, 0xD0, 0x41, 0x01, 0x00,
  };

  DrulesFormat f;
  TEST(DecodeDrules(std::string_view(reinterpret_cast<char const *>(kBlob), sizeof(kBlob)), f), ());

  TEST_EQUAL(f.VariantCount(), 1, ());
  TEST_EQUAL(f.variants.at(0), "light", ());
  TEST_EQUAL(f.types.size(), 1, ());

  TypeEntry const & t = f.types.at(0);
  TEST_EQUAL(t.name, "highway-primary", ());
  TEST_EQUAL(t.elements.size(), 1, ());

  Element const & e = t.elements.at(0);
  TEST_EQUAL(e.scale, 10, ());
  TEST_EQUAL(e.lines.size(), 1, ());

  LineRule const & line = e.lines.at(0);
  TEST_EQUAL(line.width, 2.0f, ());
  TEST_EQUAL(line.priority, 100, ());
  TEST(line.join == LineJoin::Bevel, ());
  TEST(line.cap == LineCap::Square, ());
  // Color fields are palette indices; resolve via the (single) variant's palette.
  TEST_EQUAL(f.colors.at(0).at(line.color), 0xFF0000u, ());

  TEST(e.caption.has_value(), ());
  TEST(e.caption->primary.has_value(), ());
  TEST_EQUAL(e.caption->primary->height, 12, ());
  TEST_EQUAL(e.caption->primary->text, "name", ());
  TEST_EQUAL(e.caption->priority, 4200, ());
  TEST_EQUAL(f.colors.at(0).at(e.caption->primary->color), 0x010203u, ());

  TEST_EQUAL(f.namedColors.size(), 1, ());
  TEST_EQUAL(f.namedColors.at(0).name, "Foreground", ());
  TEST_EQUAL(f.colors.at(0).at(f.namedColors.at(0).colorIdx), 0x202020u, ());
}
}  // namespace drules_format_tests

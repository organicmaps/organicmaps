#pragma once

#include "indexer/drules_struct.hpp"

#include <array>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace drule
{
// Native drawing-rules binary format, produced by tools/kothic (Python) and decoded here.
// A single structural description (types and their per-zoom draw elements) is shared by all style
// variants of a family (light/dark); only the color palette differs between variants. See
// plans/drules-protobuf-removal.md for the exact byte-level layout.
//
// Wire conventions: little-endian; varuint == LEB128 (coding/varint.hpp WriteVarUint/ReadVarUint);
// signed ints == zigzag varint (WriteVarInt); f32 == IEEE-754 bits stored as u32; strings ==
// varuint length + UTF-8; string/color references == varuint indices.
//
// Color indices: every color field in the decoded structs below is a PALETTE INDEX into
// DrulesFormat::colors[variant]; the loader resolves it to ARGB per variant. String references
// are resolved to std::string at decode time.

inline constexpr char kFormatMagic[4] = {'O', 'M', 'D', 'R'};
inline constexpr uint8_t kFormatVersion = 1;

// LineDef/LineRule presence flags.
inline constexpr uint8_t kLineFlagDashDot = 1 << 0;
inline constexpr uint8_t kLineFlagPathSym = 1 << 1;
// AreaRule presence flags.
inline constexpr uint8_t kAreaFlagBorder = 1 << 0;
// CaptionRule/PathTextRule presence flags.
inline constexpr uint8_t kCaptionFlagPrimary = 1 << 0;
inline constexpr uint8_t kCaptionFlagSecondary = 1 << 1;
// Element kindMask bits. The order also defines the on-disk payload order, which must match the
// rule emission order in the loader (lines, area, symbol, caption, path_text, shield).
inline constexpr uint8_t kKindLines = 1 << 0;
inline constexpr uint8_t kKindArea = 1 << 1;
inline constexpr uint8_t kKindSymbol = 1 << 2;
inline constexpr uint8_t kKindCaption = 1 << 3;
inline constexpr uint8_t kKindPathText = 1 << 4;
inline constexpr uint8_t kKindShield = 1 << 5;

// RuleCounts section order, used to reserve() per-kind storage in the loader.
enum RuleKind
{
  kRuleLines = 0,
  kRuleAreas,
  kRuleSymbols,
  kRuleCaptions,
  kRulePathTexts,
  kRuleShields,
  kRuleKindCount
};

// One z-level set of draw rules for a single classificator type.
struct Element
{
  uint8_t scale = 0;
  std::vector<std::string> applyIf;  // runtime selector expressions (ParseSelector input)

  std::vector<LineRule> lines;
  std::optional<AreaRule> area;
  std::optional<SymbolRule> symbol;
  std::optional<CaptionRule> caption;
  std::optional<PathTextRule> pathText;
  std::optional<ShieldRule> shield;

  bool operator==(Element const &) const = default;
};

struct TypeEntry
{
  std::string name;  // e.g. "highway-residential"
  std::vector<Element> elements;

  bool operator==(TypeEntry const &) const = default;
};

// Full replacement of a single element for one variant (escape hatch for a style whose
// light/dark variants diverge in more than colors; empty in current data).
struct Override
{
  uint32_t typeIdx = 0;
  uint32_t elemIdx = 0;
  Element element;

  bool operator==(Override const &) const = default;
};

struct NamedColor
{
  std::string name;
  uint32_t colorIdx = 0;

  bool operator==(NamedColor const &) const = default;
};

struct DrulesFormat
{
  std::vector<std::string> variants;  // "light","dark" | "design"
  // colors[variantIdx][colorIdx] == ARGB. Same colorIdx is the same color role across variants.
  std::vector<std::vector<uint32_t>> colors;
  std::vector<NamedColor> namedColors;  // mapcss colors{} block
  // Total rule count per kind (RuleKind order), for exact reserve() => stable rule pointers.
  std::array<uint32_t, kRuleKindCount> ruleCounts{};
  std::vector<TypeEntry> types;                  // sorted by name (writer-enforced)
  std::vector<std::vector<Override>> overrides;  // [variantIdx] -> overrides

  size_t VariantCount() const { return variants.size(); }

  bool operator==(DrulesFormat const &) const = default;
};

// Decodes the whole blob into out. Returns false (leaving out partially filled) when the magic
// or format version don't match, so the caller can fall back to a bundled file. A corrupt body trips
// a DEBUG-only ASSERT (corrupt drules are a build/designer error, not a runtime condition); release
// trusts the file, though a truncated read can still surface as a Reader exception.
bool DecodeDrules(std::string_view blob, DrulesFormat & out);
}  // namespace drule

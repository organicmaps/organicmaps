#pragma once

#include "base/buffer_vector.hpp"

#include <cstdint>
#include <optional>
#include <string>

namespace drule
{
// Runtime drawing-rule structs consumed by the renderer. Field names are snake_case to match the
// MapCSS style vocabulary the data is generated from.
//
// Color fields (`color`, `stroke_color`, `text_color`, `text_stroke_color`) are uint32_t.
// In a freshly decoded DrulesFormat they hold a PALETTE INDEX (see drules_format.hpp); the
// loader resolves them to final ARGB per style variant. At runtime (inside RulesHolder)
// they always hold ARGB.

// These integer values are persisted in the binary drules format and mapped to dp::* in
// apply_feature_functors.cpp, so they must stay stable.
enum class LineJoin : uint8_t
{
  Round = 0,  // was ROUNDJOIN
  Bevel = 1,  // was BEVELJOIN
  No = 2,     // was NOJOIN
};

enum class LineCap : uint8_t
{
  Round = 0,   // was ROUNDCAP
  Butt = 1,    // was BUTTCAP
  Square = 2,  // was SQUARECAP
};

struct PathSym
{
  std::string name;
  float step = 0;
  float offset = 0;

  bool operator==(PathSym const &) const = default;
};

struct DashDot
{
  buffer_vector<float, 4> dd;
  float offset = 0;

  bool operator==(DashDot const &) const = default;
};

struct LineDef
{
  float width = 0;
  uint32_t color = 0;
  std::optional<DashDot> dashdot;
  std::optional<PathSym> pathsym;
  LineJoin join = LineJoin::Round;
  LineCap cap = LineCap::Round;

  bool operator==(LineDef const &) const = default;
};

struct LineRule : LineDef
{
  int32_t priority = 0;

  bool operator==(LineRule const &) const = default;
};

struct AreaRule
{
  uint32_t color = 0;
  std::optional<LineDef> border;
  int32_t priority = 0;

  bool operator==(AreaRule const &) const = default;
};

struct SymbolRule
{
  std::string name;
  int32_t apply_for_type = 0;  // 1 - for nodes, 2 - for ways, default - for all
  int32_t priority = 0;
  int32_t min_distance = 0;

  bool operator==(SymbolRule const &) const = default;
};

struct CaptionDef
{
  int32_t height = 0;
  uint32_t color = 0;
  uint32_t stroke_color = 0;
  int32_t offset_x = 0;
  int32_t offset_y = 0;
  std::string text;
  bool is_optional = false;

  bool operator==(CaptionDef const &) const = default;
};

struct CaptionRule
{
  std::optional<CaptionDef> primary;
  std::optional<CaptionDef> secondary;
  int32_t priority = 0;

  bool operator==(CaptionRule const &) const = default;
};

// PathText has the very same shape as Caption (two optional CaptionDefs + priority).
using PathTextRule = CaptionRule;

struct ShieldRule
{
  int32_t height = 0;
  uint32_t color = 0;
  uint32_t stroke_color = 0;
  int32_t priority = 0;
  int32_t min_distance = 0;
  uint32_t text_color = 0;
  uint32_t text_stroke_color = 0;

  bool operator==(ShieldRule const &) const = default;
};
}  // namespace drule

#pragma once

#include "indexer/drawing_rule_def.hpp"
#include "indexer/drules_format.hpp"
#include "indexer/drules_selector.hpp"
#include "indexer/drules_struct.hpp"
#include "indexer/map_style.hpp"

#include "base/string_utils.hpp"

#include <deque>
#include <functional>
#include <memory>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

class FeatureType;
class Classificator;

namespace drule
{
class BaseRule
{
public:
  BaseRule() = default;
  virtual ~BaseRule() = default;
  // Move-only because each rule owns its runtime selector.
  BaseRule(BaseRule &&) = default;
  BaseRule & operator=(BaseRule &&) = default;

  virtual LineRule const * GetLine() const { return nullptr; }
  virtual AreaRule const * GetArea() const { return nullptr; }
  virtual SymbolRule const * GetSymbol() const { return nullptr; }
  virtual CaptionRule const * GetCaption() const { return nullptr; }
  virtual PathTextRule const * GetPathtext() const { return nullptr; }
  virtual ShieldRule const * GetShield() const { return nullptr; }

  // Test feature by runtime feature style selector
  // Returns true if rule is applicable for feature, otherwise it returns false
  bool TestFeature(FeatureType & ft, int zoom) const;

  // Set runtime feature style selector
  void SetSelector(std::unique_ptr<ISelector> && selector);

private:
  std::unique_ptr<ISelector> m_selector;
};

// Per-kind wrappers are stored by value in deques. Appending rules preserves existing addresses;
// pointers returned by Find() remain valid until the holder is reloaded or destroyed.
struct LineRuleHolder final : BaseRule
{
  LineRule m_rule;
  LineRule const * GetLine() const override { return &m_rule; }
};
struct AreaRuleHolder final : BaseRule
{
  AreaRule m_rule;
  AreaRule const * GetArea() const override { return &m_rule; }
};
struct SymbolRuleHolder final : BaseRule
{
  SymbolRule m_rule;
  SymbolRule const * GetSymbol() const override { return &m_rule; }
};
struct CaptionRuleHolder final : BaseRule
{
  CaptionRule m_rule;
  CaptionRule const * GetCaption() const override { return &m_rule; }
};
struct PathTextRuleHolder final : BaseRule
{
  PathTextRule m_rule;
  PathTextRule const * GetPathtext() const override { return &m_rule; }
};
struct ShieldRuleHolder final : BaseRule
{
  ShieldRule m_rule;
  ShieldRule const * GetShield() const override { return &m_rule; }
};

class RulesHolder
{
public:
  RulesHolder();
  ~RulesHolder();

  // Returns a non-null rule for a key from this holder's current load.
  BaseRule const * Find(Key const & k) const;

  uint32_t GetBgColor(int scale) const;
  uint32_t GetColor(std::string_view name) const;

  // Fills this holder (and classifTree's draw rules) from one variant of a decoded family. Colors
  // are resolved to ARGB using that variant's palette. classifTree must be the tree of the same
  // style as this holder. Reloading invalidates rule pointers; no readers may access this holder
  // or classifTree while they are being filled.
  void LoadFromFormat(DrulesFormat const & fmt, size_t variant, Classificator & classifTree);

  bool IsEmpty() const { return m_dRules.empty(); }

  template <class ToDo>
  void ForEachRule(ToDo && toDo)
  {
    for (auto const dRule : m_dRules)
      toDo(dRule);
  }

private:
  friend class RulesLoader;

  void InitBackgroundColors(DrulesFormat const & fmt, size_t variant);
  void InitColors(DrulesFormat const & fmt, size_t variant);
  void Clean();

  /// background color for scales in range [0...scales::UPPER_STYLE_SCALE]
  std::vector<uint32_t> m_bgColors;
  std::unordered_map<std::string, uint32_t, base::StringHash, std::equal_to<>> m_colors;

  // Appending to these deques preserves m_dRules pointers; Clean() invalidates them.
  std::deque<LineRuleHolder> m_lineRules;
  std::deque<AreaRuleHolder> m_areaRules;
  std::deque<SymbolRuleHolder> m_symbolRules;
  std::deque<CaptionRuleHolder> m_captionRules;
  std::deque<PathTextRuleHolder> m_pathtextRules;
  std::deque<ShieldRuleHolder> m_shieldRules;

  std::vector<BaseRule *> m_dRules;
};

RulesHolder & GetCurrentRules();
// Accessor for a specific style's holder, for the loader and tile readers that must not observe a
// concurrent style switch.
RulesHolder & GetRules(MapStyle mapStyle);

// Decodes mapStyle's drawing-rules file (a valid WritableDir override if present, else the bundled
// one). The family file holds all variants; pass the result to RulesHolder::LoadFromFormat per
// variant so one decode fills the whole family.
DrulesFormat DecodeRules(MapStyle mapStyle);
}  // namespace drule

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
  // Move-only (holds a unique_ptr selector); stored by value in RulesHolder's per-kind vectors.
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

// Thin per-kind BaseRule wrappers. They are stored by value in RulesHolder's per-kind deques, which
// never relocate their elements, so the raw pointers handed out via Find() stay stable for the
// holder's lifetime.
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

  BaseRule const * Find(Key const & k) const;

  uint32_t GetBgColor(int scale) const;
  uint32_t GetColor(std::string_view name) const;

  // Fills this holder (and classifTree's draw rules) from one variant of a decoded family. Colors
  // are resolved to ARGB using that variant's palette. classifTree must be the tree of the same
  // style as this holder.
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

  // Per-kind storage; std::deque never relocates elements, so the m_dRules pointers stay stable.
  std::deque<LineRuleHolder> m_lineRules;
  std::deque<AreaRuleHolder> m_areaRules;
  std::deque<SymbolRuleHolder> m_symbolRules;
  std::deque<CaptionRuleHolder> m_captionRules;
  std::deque<PathTextRuleHolder> m_pathtextRules;
  std::deque<ShieldRuleHolder> m_shieldRules;

  std::vector<BaseRule *> m_dRules;
};

RulesHolder & GetCurrentRules();
RulesHolder & GetOutdoorRules();
// Accessor for a specific style's holder; used by the loader to fill a family without changing the
// global current style.
RulesHolder & GetRules(MapStyle mapStyle);

// Decodes mapStyle's drawing-rules file (a valid WritableDir override if present, else the bundled
// one). The family file holds all variants; pass the result to RulesHolder::LoadFromFormat per
// variant so one decode fills the whole family.
DrulesFormat DecodeRules(MapStyle mapStyle);
}  // namespace drule

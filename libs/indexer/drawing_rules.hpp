#pragma once

#include "indexer/drawing_rule_def.hpp"
#include "indexer/drules_selector.hpp"
#include "indexer/map_style.hpp"

#include "base/base.hpp"
#include "base/buffer_vector.hpp"

#include "std/target_os.hpp"

#include <array>
#include <iostream>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

class LineRuleProto;
class AreaRuleProto;
class SymbolRuleProto;
class CaptionRuleProto;
class PathTextRuleProto;
class ShieldRuleProto;
class ContainerProto;
class FeatureType;

namespace drule
{
class BaseRule
{
public:
  BaseRule() = default;
  virtual ~BaseRule() = default;

  virtual LineRuleProto const * GetLine() const;
  virtual AreaRuleProto const * GetArea() const;
  virtual SymbolRuleProto const * GetSymbol() const;
  virtual CaptionRuleProto const * GetCaption() const;
  virtual PathTextRuleProto const * GetPathtext() const;
  virtual ShieldRuleProto const * GetShield() const;

  // Test feature by runtime feature style selector
  // Returns true if rule is applicable for feature, otherwise it returns false
  bool TestFeature(FeatureType & ft, int zoom) const;

  // Set runtime feature style selector
  void SetSelector(std::unique_ptr<ISelector> && selector);

private:
  std::unique_ptr<ISelector> m_selector;
};

class RulesHolder
{
public:
  RulesHolder();
  ~RulesHolder();

  Key AddRule(int scale, TypeT type, BaseRule * p);

  BaseRule const * Find(Key const & k) const;

  uint32_t GetBgColor(int scale) const;
  uint32_t GetColor(std::string const & name) const;

#ifdef OMIM_OS_DESKTOP
  void LoadFromTextProto(std::string const & buffer);
  static void SaveToBinaryProto(std::string const & buffer, std::ostream & s);
#endif

  void LoadFromBinaryProto(std::string const & s);

  template <class ToDo>
  void ForEachRule(ToDo && toDo)
  {
    for (auto const dRule : m_dRules)
      toDo(dRule);
  }

private:
  void InitBackgroundColors(ContainerProto const & cp);
  void InitColors(ContainerProto const & cp);
  void Clean();

  /// background color for scales in range [0...scales::UPPER_STYLE_SCALE]
  std::vector<uint32_t> m_bgColors;
  std::unordered_map<std::string, uint32_t> m_colors;
  std::vector<BaseRule *> m_dRules;
};

RulesHolder & rules();

void LoadRules();
}  // namespace drule

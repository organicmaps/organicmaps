#pragma once

#include "indexer/drawing_rule_def.hpp"
#include "indexer/drawing_rules.hpp"
#include "indexer/ftypes_matcher.hpp"
#include "indexer/road_shields_parser.hpp"

#include "base/buffer_vector.hpp"

#include <functional>
#include <string>

class FeatureType;

namespace drule
{
class BaseRule;
}

namespace df
{

class IsHatchingTerritoryChecker : public ftypes::BaseChecker
{
  IsHatchingTerritoryChecker();

public:
  DECLARE_CHECKER_INSTANCE(IsHatchingTerritoryChecker);

protected:
  bool IsMatched(uint32_t type) const override;

private:
  size_t m_type3end;
};

struct CaptionDescription
{
  void Init(FeatureType & f, int8_t deviceLang, int zoomLevel, feature::GeomType geomType, bool auxCaptionExists);

  std::string const & GetMainText() const { return m_mainText; }
  std::string const & GetAuxText() const { return m_auxText; }
  std::string const & GetHouseNumberText() const { return m_houseNumberText; }

  bool IsNameExists() const { return !m_mainText.empty(); }
  bool IsHouseNumberExists() const { return !m_houseNumberText.empty(); }

private:
  std::string m_mainText;
  std::string m_auxText;
  std::string m_houseNumberText;
};

class Stylist
{
public:
  SymbolRuleProto const * m_symbolRule = nullptr;
  CaptionRuleProto const * m_captionRule = nullptr;
  CaptionRuleProto const * m_houseNumberRule = nullptr;
  PathTextRuleProto const * m_pathtextRule = nullptr;
  ShieldRuleProto const * m_shieldRule = nullptr;
  AreaRuleProto const * m_areaRule = nullptr;
  AreaRuleProto const * m_hatchingRule = nullptr;

  using LineRulesT = buffer_vector<LineRuleProto const *, 4>;
  LineRulesT m_lineRules;

  ftypes::RoadShieldsSetT m_roadShields;

  Stylist(FeatureType & f, uint8_t zoomLevel, int8_t deviceLang);

  CaptionDescription const & GetCaptionDescription() const { return m_captionDescriptor; }

private:
  void ProcessKey(FeatureType & f, drule::Key const & key);

  CaptionDescription m_captionDescriptor;
};

}  // namespace df

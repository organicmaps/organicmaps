#pragma once

#include "indexer/drawing_rules.hpp"
#include "indexer/ftypes_matcher.hpp"
#include "indexer/road_shields_parser.hpp"

#include "base/buffer_vector.hpp"

#include <string>

class FeatureType;

namespace df
{

class IsHatchingTerritoryChecker
{
  IsHatchingTerritoryChecker();

protected:
  void PostInitialize() { m_2level45.PostInitialize(); }

public:
  DECLARE_CHECKER_INSTANCE(IsHatchingTerritoryChecker);

  std::string_view GetHatch(uint32_t type) const;
  std::string_view GetHatch(feature::TypesHolder const & types) const;

  template <class T>
  bool operator()(T && t) const
  {
    return !GetHatch(t).empty();
  }

private:
  // 45d hatch.
  struct TwoLevel45 : ftypes::BaseChecker
  {
    TwoLevel45();
  } m_2level45;
  uint32_t m_3level45;

  uint32_t m_2levelDash;  // dash hatch
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

  CaptionDescription m_captionDescriptor;

  Stylist(FeatureType & f, uint8_t zoomLevel, int8_t deviceLang, bool forceOutdoorStyle);

private:
  void ProcessKey(FeatureType & f, drule::Key const & key);

  drule::RulesHolder const & m_rulesHolder;
};

}  // namespace df

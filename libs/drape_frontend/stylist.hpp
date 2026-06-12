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
  struct TwoLevel45 : ftypes::BaseCheckerEx
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
  // StringUtf8Multilang code of the language actually selected for main/aux text. Drives
  // HarfBuzz OpenType `locl` substitutions downstream. kUnsupportedLanguageCode when empty.
  int8_t GetMainTextLang() const { return m_mainTextLang; }
  int8_t GetAuxTextLang() const { return m_auxTextLang; }
  // First language declared in the feature's MWM region metadata, or kUnsupportedLanguageCode
  // when the MWM declares none. Use as the locl hint for OSM-verbatim text that does not go
  // through name selection (addr:housenumber, road shield ref tags).
  int8_t GetMwmRegionLang() const { return m_mwmRegionLang; }

  bool IsNameExists() const { return !m_mainText.empty(); }
  bool IsHouseNumberExists() const { return !m_houseNumberText.empty(); }

private:
  std::string m_mainText;
  std::string m_auxText;
  std::string m_houseNumberText;
  int8_t m_mainTextLang = StringUtf8Multilang::kUnsupportedLanguageCode;
  int8_t m_auxTextLang = StringUtf8Multilang::kUnsupportedLanguageCode;
  int8_t m_mwmRegionLang = StringUtf8Multilang::kUnsupportedLanguageCode;
};

class Stylist
{
public:
  drule::SymbolRule const * m_symbolRule = nullptr;
  drule::CaptionRule const * m_captionRule = nullptr;
  drule::CaptionRule const * m_houseNumberRule = nullptr;
  drule::PathTextRule const * m_pathtextRule = nullptr;
  drule::ShieldRule const * m_shieldRule = nullptr;
  drule::AreaRule const * m_areaRule = nullptr;
  drule::AreaRule const * m_hatchingRule = nullptr;

  using LineRulesT = buffer_vector<drule::LineRule const *, 4>;
  LineRulesT m_lineRules;

  ftypes::RoadShieldsSetT m_roadShields;

  CaptionDescription m_captionDescriptor;

  Stylist(FeatureType & f, uint8_t zoomLevel, int8_t deviceLang, bool forceOutdoorStyle);

private:
  void ProcessKey(FeatureType & f, drule::Key const & key);

  drule::RulesHolder const & m_rulesHolder;
};

}  // namespace df

#pragma once

#include "indexer/ftypes_matcher.hpp"
#include "indexer/drawing_rule_def.hpp"
#include "indexer/drawing_rules.hpp"

#include "base/buffer_vector.hpp"

#include <functional>
#include <string>

class FeatureType;

namespace drule { class BaseRule; }

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
  bool m_isCoastline = false;

  drule::BaseRule const * m_symbolRule = nullptr;
  drule::BaseRule const * m_captionRule = nullptr;
  drule::BaseRule const * m_houseNumberRule = nullptr;
  drule::BaseRule const * m_pathtextRule = nullptr;
  drule::BaseRule const * m_shieldRule = nullptr;
  drule::BaseRule const * m_areaRule = nullptr;
  drule::BaseRule const * m_hatchingRule = nullptr;
  buffer_vector<drule::BaseRule const *, 4> m_lineRules;

  Stylist(FeatureType & f, uint8_t zoomLevel, int8_t deviceLang);

  CaptionDescription const & GetCaptionDescription() const { return m_captionDescriptor; }

private:
  void ProcessKey(FeatureType & f, drule::Key const & key);

  CaptionDescription m_captionDescriptor;
};

}  // namespace df

#pragma once

#include "indexer/ftypes_matcher.hpp"
#include "indexer/drawing_rule_def.hpp"

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

struct TRuleWrapper
{
  drule::BaseRule const * m_rule;
  float m_depth;
  bool m_hatching;
  bool m_isHouseNumber;
};

struct CaptionDescription
{
  void Init(FeatureType & f, int8_t deviceLang, int const zoomLevel,
            feature::GeomType const type, bool const auxCaptionExists);

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
  bool m_areaStyleExists = false;
  bool m_lineStyleExists = false;
  bool m_pointStyleExists = false;

  CaptionDescription const & GetCaptionDescription() const { return m_captionDescriptor; }

  template <class ToDo> void ForEachRule(ToDo && toDo) const
  {
    for (auto const & r : m_rules)
      toDo(r);
  }

  bool IsEmpty() const { return m_rules.empty(); }

private:
  friend bool InitStylist(FeatureType & f, int8_t deviceLang, int const zoomLevel, bool buildings3d,
                          Stylist & s);

  typedef buffer_vector<TRuleWrapper, 8> rules_t;
  rules_t m_rules;

  CaptionDescription m_captionDescriptor;
};

bool InitStylist(FeatureType & f, int8_t deviceLang, int const zoomLevel, bool buildings3d,
                 Stylist & s);
}  // namespace df

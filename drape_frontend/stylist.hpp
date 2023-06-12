#pragma once

#include "indexer/ftypes_matcher.hpp"
#include "indexer/drawing_rule_def.hpp"

#include "base/buffer_vector.hpp"

#include <cstdint>
#include <functional>
#include <string>
#include <utility>

class FeatureType;

namespace drule { class BaseRule; }

namespace df
{

// Priority range for area and line drules. Each layer = +/-1 value shifts the range by this number,
// so that e.g. priorities of the default layer=0 range [0;1000) don't intersect with layer=-1 range [-1000;0) and so on..
double constexpr kLayerDepthRange = 1000;

class IsHatchingTerritoryChecker : public ftypes::BaseChecker
{
  IsHatchingTerritoryChecker();
public:
  DECLARE_CHECKER_INSTANCE(IsHatchingTerritoryChecker);
};

struct CaptionDescription
{
  void Init(FeatureType & f, int8_t deviceLang, int const zoomLevel, feature::GeomType const type,
            drule::text_type_t const mainTextType, bool const auxCaptionExists);

  std::string const & GetMainText() const;
  std::string const & GetAuxText() const;
  bool IsNameExists() const;
  bool IsHouseNumberInMainText() const { return m_isHouseNumberInMainText; }

private:
  // Clear aux name on high zoom and clear long main name on low zoom.
  void ProcessZoomLevel(int const zoomLevel);
  // Try to use house number as name of the object.
  void ProcessMainTextType(drule::text_type_t const & mainTextType);

  std::string m_mainText;
  std::string m_auxText;
  std::string m_houseNumber;
  bool m_isHouseNumberInMainText = false;
};

class Stylist
{
public:
  bool m_isCoastline = false;
  bool m_areaStyleExists = false;
  bool m_lineStyleExists = false;
  bool m_pointStyleExists = false;

public:
  CaptionDescription const & GetCaptionDescription() const;

  struct TRuleWrapper
  {
    drule::BaseRule const * m_rule;
    float m_depth;
    bool m_hatching;
  };

  template <class ToDo> void ForEachRule(ToDo && toDo) const
  {
    for (auto const & r : m_rules)
      toDo(r);
  }

  bool IsEmpty() const;

private:
  friend bool InitStylist(FeatureType & f, int8_t deviceLang, int const zoomLevel, bool buildings3d,
                          Stylist & s);

  CaptionDescription & GetCaptionDescriptionImpl();

private:
  typedef buffer_vector<TRuleWrapper, 8> rules_t;
  rules_t m_rules;

  CaptionDescription m_captionDescriptor;
};

bool InitStylist(FeatureType & f, int8_t deviceLang, int const zoomLevel, bool buildings3d,
                 Stylist & s);
}  // namespace df

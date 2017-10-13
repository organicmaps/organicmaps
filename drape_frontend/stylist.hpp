#pragma once

#include "indexer/feature_data.hpp"
#include "indexer/ftypes_matcher.hpp"
#include "indexer/drawing_rule_def.hpp"

#include "base/buffer_vector.hpp"

#include "std/function.hpp"
#include "std/string.hpp"

class FeatureType;

namespace drule { class BaseRule; }

namespace df
{
class IsBuildingHasPartsChecker : public ftypes::BaseChecker
{
  IsBuildingHasPartsChecker();
public:
  DECLARE_CHECKER_INSTANCE(IsBuildingHasPartsChecker);
};

class IsBuildingPartChecker : public ftypes::BaseChecker
{
  IsBuildingPartChecker();
public:
  DECLARE_CHECKER_INSTANCE(IsBuildingPartChecker);
};

class IsHatchingTerritoryChecker : public ftypes::BaseChecker
{
  IsHatchingTerritoryChecker();
public:
  DECLARE_CHECKER_INSTANCE(IsHatchingTerritoryChecker);
};

struct CaptionDescription
{
  void Init(FeatureType const & f,
            int8_t deviceLang,
            int const zoomLevel,
            feature::EGeomType const type,
            drule::text_type_t const mainTextType,
            bool const auxCaptionExists);

  string const & GetMainText() const;
  string const & GetAuxText() const;
  string const & GetRoadNumber() const;
  bool IsNameExists() const;

private:
  /// Clear aux name on high zoom and clear long main name on low zoom.
  void ProcessZoomLevel(int const zoomLevel);
  /// Try to use house number as name of the object.
  void ProcessMainTextType(drule::text_type_t const & mainTextType);

  string m_mainText;
  string m_auxText;
  string m_roadNumber;
  string m_houseNumber;
};

class Stylist
{
public:
  Stylist();

  bool IsCoastLine() const;
  bool AreaStyleExists() const;
  bool LineStyleExists() const;
  bool PointStyleExists() const;

  CaptionDescription const & GetCaptionDescription() const;

  using TRuleWrapper = pair<drule::BaseRule const *, double>;
  using TRuleCallback = function<void (TRuleWrapper const &)>;
  void ForEachRule(TRuleCallback const & fn) const;

  bool IsEmpty() const;

private:
  friend bool InitStylist(FeatureType const & f,
                          int8_t deviceLang,
                          int const zoomLevel,
                          bool buildings3d,
                          Stylist & s);

  void RaiseCoastlineFlag();
  void RaiseAreaStyleFlag();
  void RaiseLineStyleFlag();
  void RaisePointStyleFlag();

  CaptionDescription & GetCaptionDescriptionImpl();

private:
  typedef buffer_vector<TRuleWrapper, 8> rules_t;
  rules_t m_rules;

  uint8_t m_state;
  CaptionDescription m_captionDescriptor;
};

bool InitStylist(FeatureType const & f,
                 int8_t deviceLang,
                 int const zoomLevel,
                 bool buildings3d,
                 Stylist & s);

double GetFeaturePriority(FeatureType const & f, int const zoomLevel);

} // namespace df

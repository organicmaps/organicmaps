#pragma once

#include "indexer/feature_data.hpp"

#include "base/buffer_vector.hpp"

#include "std/function.hpp"
#include "std/string.hpp"

class FeatureType;

namespace drule { class BaseRule; }

namespace df
{

struct CaptionDescription
{
  CaptionDescription();

  void Init(FeatureType const & f,
            int const zoomLevel);

  void FormatCaptions(FeatureType const & f,
                      feature::EGeomType type,
                      bool auxCaptionExists);

  string const & GetMainText() const;
  string const & GetAuxText() const;
  string const & GetRoadNumber() const;
  string GetPathName() const;
  double GetPopulationRank() const;
  bool IsNameExists() const;

private:
  void SwapCaptions(int const zoomLevel);
  void DiscardLongCaption(int const zoomLevel);

private:
  string m_mainText;
  string m_auxText;
  string m_roadNumber;
  string m_houseNumber;
  double m_populationRank;
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
  void ForEachRule(TRuleCallback const & fn);

  bool IsEmpty() const;

private:
  friend bool InitStylist(FeatureType const &,
                          int const,
                          Stylist &);

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
                 int const zoomLevel,
                 Stylist & s);

} // namespace df

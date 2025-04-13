#pragma once

#include "generator/filter_interface.hpp"

#include "indexer/ftypes_matcher.hpp"

#include <string>

namespace generator
{
class FilterWorld : public FilterInterface
{
public:
  explicit FilterWorld(std::string const & popularityFilename);

  // FilterInterface overrides:
  std::shared_ptr<FilterInterface> Clone() const override;

  bool IsAccepted(feature::FeatureBuilder const & feature) const override;

  static bool IsInternationalAirport(feature::FeatureBuilder const & fb);
  static bool IsGoodScale(feature::FeatureBuilder const & fb);
  static bool IsPopularAttraction(feature::FeatureBuilder const & fb, std::string const & popularityFilename);

private:
  std::string m_popularityFilename;
  ftypes::IsCityTownOrVillageChecker const & m_isCityTownVillage;
};
}  // namespace generator

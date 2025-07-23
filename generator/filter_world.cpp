#include "generator/filter_world.hpp"

#include "generator/popular_places_section_builder.hpp"

#include "indexer/classificator.hpp"
#include "indexer/feature_visibility.hpp"
#include "indexer/ftypes_matcher.hpp"
#include "indexer/scales.hpp"

#include <algorithm>

namespace generator
{
FilterWorld::FilterWorld(std::string const & popularityFilename)
  : m_popularityFilename(popularityFilename)
  , m_isCityTownVillage(ftypes::IsCityTownOrVillageChecker::Instance())
{
  if (popularityFilename.empty())
    LOG(LWARNING, ("popular_places_data option not set. Popular attractions will not be added to World.mwm"));
}

std::shared_ptr<FilterInterface> FilterWorld::Clone() const
{
  return std::make_shared<FilterWorld>(m_popularityFilename);
}

bool FilterWorld::IsAccepted(feature::FeatureBuilder const & fb) const
{
  // We collect and process localities in ProcessorCities.
  /// @todo What if some fancy FBs like town and airport simultaneously? :)
  if (m_isCityTownVillage(fb.GetTypes()))
    return false;

  return (IsGoodScale(fb) || IsPopularAttraction(fb, m_popularityFilename) || IsInternationalAirport(fb));
}

// static
bool FilterWorld::IsInternationalAirport(feature::FeatureBuilder const & fb)
{
  auto static const kAirport = classif().GetTypeByPath({"aeroway", "aerodrome", "international"});
  return fb.HasType(kAirport);
}

// static
bool FilterWorld::IsGoodScale(feature::FeatureBuilder const & fb)
{
  // GetMinDrawableScale also checks suitable size for AREA features.
  int const minScale = GetMinDrawableScale(fb.GetTypesHolder(), fb.GetLimitRect());

  // Some features become invisible after merge processing, so -1 is possible.
  return scales::GetUpperWorldScale() >= minScale && minScale != -1;
}

// static
bool FilterWorld::IsPopularAttraction(feature::FeatureBuilder const & fb, std::string const & popularityFilename)
{
  if (fb.GetName().empty())
    return false;

  auto const & attractionsChecker = ftypes::AttractionsChecker::Instance();
  if (!attractionsChecker(fb.GetTypes()))
    return false;

  if (popularityFilename.empty())
    return false;

  auto static const & m_popularPlaces = GetOrLoadPopularPlaces(popularityFilename);
  auto const it = m_popularPlaces.find(fb.GetMostGenericOsmId());
  if (it == m_popularPlaces.end())
    return false;

  // todo(@t.yan): adjust
  uint8_t const kPopularityThreshold = 13;
  // todo(@t.yan): maybe check place has wikipedia link.
  return it->second >= kPopularityThreshold;
}
}  // namespace generator

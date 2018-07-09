#pragma once

#include <string>

class FeatureBuilder1;

namespace generator
{
namespace impl
{
double GetLinearNormDistanceScore(double distance, double maxDistance);
double GetNameSimilarityScore(std::string const & booking_name, std::string const & osm_name);
}  // namespace impl

namespace sponsored_scoring
{
/// Represents a match scoring statystics of a sponsored object agains osm object.
template <typename SponsoredObject>
struct MatchStats
{
  /// Returns some score based on geven fields and classificator tuning.
  double GetMatchingScore() const;
  /// Returns true if GetMatchingScore is greater then some theshold.
  bool IsMatched() const;

  double m_linearNormDistanceScore{};
  double m_nameSimilarityScore{};
};

/// Matches a given sponsored object against a given OSM object.
template <typename SponsoredObject>
MatchStats<SponsoredObject> Match(SponsoredObject const & o, FeatureBuilder1 const & fb);
}  // namespace booking_scoring
}  // namespace generator

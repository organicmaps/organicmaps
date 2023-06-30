#pragma once

#include <string>

namespace generator
{
struct SponsoredObjectBase;

namespace sponsored
{

/// Represents a match scoring statistics of a sponsored object against OSM object.
class MatchStats
{
  // Calculated with tools/python/booking_hotels_quality.py.
  static double constexpr kOptimalThreshold = 0.304875;

public:
  MatchStats(double distM, double distLimitM, std::string const & name, std::string const & fbName);

  /// @return some score based on geven fields and classificator tuning.
  double GetMatchingScore() const
  {
    // TODO(mgsergio): Use tuner to get optimal function.
    return m_linearNormDistanceScore * m_nameSimilarityScore;
  }

  /// @return true if GetMatchingScore is greater then some theshold.
  bool IsMatched() const
  {
    return GetMatchingScore() > kOptimalThreshold;
  }

public:
  double m_distance;
  double m_linearNormDistanceScore;
  double m_nameSimilarityScore;
};

} // namespace sponsored
} // namespace generator

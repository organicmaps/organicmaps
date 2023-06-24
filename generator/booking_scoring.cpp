#include "generator/sponsored_scoring.hpp"

#include "generator/booking_dataset.hpp"
#include "generator/feature_builder.hpp"

#include "geometry/mercator.hpp"


namespace
{
// Calculated with tools/python/booking_hotels_quality.py.
double constexpr kOptimalThreshold = 0.304875;
}  // namespace

namespace generator
{
namespace sponsored_scoring
{
template <>
double MatchStats<BookingHotel>::GetMatchingScore() const
{
  // TODO(mgsergio): Use tuner to get optimal function.
  return m_linearNormDistanceScore * m_nameSimilarityScore;
}

template <>
bool MatchStats<BookingHotel>::IsMatched() const
{
  return GetMatchingScore() > kOptimalThreshold;
}

/// @todo It looks like quite common Match function implementation,
/// because GetLatLon and GetName() needed.
template <>
MatchStats<BookingHotel> Match(BookingHotel const & h, feature::FeatureBuilder const & fb)
{
  MatchStats<BookingHotel> score;

  auto const fbCenter = mercator::ToLatLon(fb.GetKeyPoint());
  auto const distance = ms::DistanceOnEarth(fbCenter, h.m_latLon);
  score.m_linearNormDistanceScore =
      impl::GetLinearNormDistanceScore(distance, BookingDataset::kDistanceLimitInMeters);

  // TODO(mgsergio): Check all translations and use the best one.
  score.m_nameSimilarityScore = impl::GetNameSimilarityScore(
      h.m_name, std::string(fb.GetName(StringUtf8Multilang::kDefaultCode)));

  return score;
}
}  // namespace sponsored_scoring
}  // namespace generator

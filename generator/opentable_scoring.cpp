#include "generator/sponsored_scoring.hpp"

#include "generator/opentable_dataset.hpp"
#include "generator/feature_builder.hpp"

using namespace feature;

namespace
{
// Calculated with tools/python/booking_hotels_quality.py.
double constexpr kOptimalThreshold = 0.312887;
}  // namespace

namespace generator
{
namespace sponsored_scoring
{
template <>
double MatchStats<OpentableRestaurant>::GetMatchingScore() const
{
  // TODO(mgsergio): Use tuner to get optimal function.
  return m_linearNormDistanceScore * m_nameSimilarityScore;
}

template <>
bool MatchStats<OpentableRestaurant>::IsMatched() const
{
  return GetMatchingScore() > kOptimalThreshold;
}

template <>
MatchStats<OpentableRestaurant> Match(OpentableRestaurant const & r, FeatureBuilder const & fb)
{
  MatchStats<OpentableRestaurant> score;

  auto const fbCenter = mercator::ToLatLon(fb.GetKeyPoint());
  auto const distance = ms::DistanceOnEarth(fbCenter, r.m_latLon);
  score.m_linearNormDistanceScore =
      impl::GetLinearNormDistanceScore(distance, OpentableDataset::kDistanceLimitInMeters);

  score.m_nameSimilarityScore =
      impl::GetNameSimilarityScore(r.m_name, fb.GetName(StringUtf8Multilang::kDefaultCode));

  return score;
}
}  // namespace sponsored_scoring
}  // namespace generator

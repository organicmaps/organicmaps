#pragma once

#include "generator/booking_dataset.hpp"

class FeatureBuilder1;

namespace generator
{
namespace booking_scoring
{
struct BookingMatchScore
{
  double GetMatchingScore() const;
  bool IsMatched() const;

  double m_linearNormDistanceScore{};
  double m_nameSimilarityScore{};
};

BookingMatchScore Match(BookingDataset::Hotel const & h, FeatureBuilder1 const & fb);
}  // namespace booking_scoring
}  // namespace generator

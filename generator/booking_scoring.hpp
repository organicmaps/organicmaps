#pragma once

#include "generator/booking_dataset.hpp"
#include "generator/osm_element.hpp"

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

BookingMatchScore Match(BookingDataset::Hotel const & h, OsmElement const & e);
}  // namespace booking_scoring
}  // namespace generator

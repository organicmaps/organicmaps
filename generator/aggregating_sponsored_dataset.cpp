#include "generator/aggregating_sponsored_dataset.hpp"

namespace generator
{
bool AggregatingSponsoredDataset::IsMatched(FeatureBuilder1 const & fb) const
{
  return m_bookingDataset.FindMatchingObjectId(fb) != BookingHotel::InvalidObjectId();
}

void AggregatingSponsoredDataset::BuildOsmObjects(function<void(FeatureBuilder1 &)> const & fn) const
{
  m_bookingDataset.BuildOsmObjects(fn);
}

size_t AggregatingSponsoredDataset::Size() const
{
  return m_bookingDataset.Size();
}
}  // namespace generator

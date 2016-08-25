#include "generator/aggregating_sponsored_dataset.hpp"

namespace generator
{
SponsoredDataset::ObjectId AggregatingSponsoredDataset::FindMatchingObjectId(FeatureBuilder1 const & fb) const
{
  // There is only one source for now.
  return m_datasets[0]->FindMatchingObjectId(fb);
}

size_t AggregatingSponsoredDataset::Size() const
{
  size_t count{};
  for (auto const & ds : m_datasets)
    count += ds->Size();
  return count;
}

void AggregatingSponsoredDataset::BuildOsmObjects(function<void(FeatureBuilder1 &)> const & fn) const
{
  for (auto const & ds : m_datasets)
    ds->BuildOsmObjects(fn);
}
}  // namespace generator

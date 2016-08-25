#pragma once

#include "generator/booking_dataset.hpp"
#include "generator/generate_info.hpp"

#include "std/unique_ptr.hpp"
#include "std/vector.hpp"

namespace generator
{
class AggregatingSponsoredDataset : public SponsoredDataset
{
public:
  explicit AggregatingSponsoredDataset(feature::GenerateInfo const & info)
  {
    m_datasets.emplace_back(make_unique<BookingDataset>(info.m_bookingDatafileName,
                                                        info.m_bookingReferenceDir));
  }

  ObjectId FindMatchingObjectId(FeatureBuilder1 const & e) const override;

  size_t Size() const override;

  void BuildOsmObjects(function<void(FeatureBuilder1 &)> const & fn) const override;

private:
  vector<unique_ptr<SponsoredDatasetBase>> m_datasets;
};
}  // namespace generator;

#pragma once

#include "generator/booking_dataset.hpp"
#include "generator/generate_info.hpp"

#include "std/unique_ptr.hpp"
#include "std/vector.hpp"

namespace generator
{
class AggregatingSponsoredDataset
{
public:
  explicit AggregatingSponsoredDataset(feature::GenerateInfo const & info)
    : m_bookingDataset(info.m_bookingDatafileName, info.m_bookingReferenceDir)
  {
  }

  bool IsMatched(FeatureBuilder1 const & e) const;
  void BuildOsmObjects(function<void(FeatureBuilder1 &)> const & fn) const;

  size_t Size() const;

private:
  BookingDataset m_bookingDataset;
};
}  // namespace generator;

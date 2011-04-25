#pragma once

#include "../geometry/rect2d.hpp"

#include "../base/base.hpp"

#include "../std/utility.hpp"
#include "../std/vector.hpp"

class FeatureType;

namespace covering
{
  // Cover feature with RectIds and return their integer representations.
  vector<int64_t> CoverFeature(FeatureType const & feature,
                               uint64_t cellPenaltyArea);
  // Cover viewport with RectIds and append their RectIds as well.
  vector<pair<int64_t, int64_t> > CoverViewportAndAppendLowerLevels(m2::RectD const & rect);
  // Given a vector of intervals [a, b), sort them and merge overlapping intervals.
  vector<pair<int64_t, int64_t> > SortAndMergeIntervals(vector<pair<int64_t, int64_t> > intervals);
}

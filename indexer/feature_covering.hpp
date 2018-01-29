#pragma once
#include "geometry/rect2d.hpp"

#include "coding/point_to_integer.hpp"

#include "std/utility.hpp"
#include "std/vector.hpp"


class FeatureType;

namespace indexer
{
class LocalityObject;
}  // namespace indexer

namespace covering
{
  typedef pair<int64_t, int64_t> IntervalT;
  typedef vector<IntervalT> IntervalsT;

  // Cover feature with RectIds and return their integer representations.
  vector<int64_t> CoverFeature(FeatureType const & feature,
                               int cellDepth,
                               uint64_t cellPenaltyArea);

  vector<int64_t> CoverLocality(indexer::LocalityObject const & o, int cellDepth,
                                uint64_t cellPenaltyArea);

  void AppendLowerLevels(RectId id, int cellDepth, IntervalsT & intervals);

  // Cover viewport with RectIds and append their RectIds as well.
  void CoverViewportAndAppendLowerLevels(m2::RectD const & rect, int cellDepth,
                                         IntervalsT & intervals);

  // Given a vector of intervals [a, b), sort them and merge overlapping intervals.
  IntervalsT SortAndMergeIntervals(IntervalsT const & intervals);

  RectId GetRectIdAsIs(m2::RectD const & r);

  // Calculate cell coding depth according to max visual scale for mwm.
  int GetCodingDepth(int scale);

  enum CoveringMode
  {
    ViewportWithLowLevels = 0,
    LowLevelsOnly,
    FullCover
  };

  class CoveringGetter
  {
    IntervalsT m_res[2];

    m2::RectD const & m_rect;
    CoveringMode m_mode;

  public:
    CoveringGetter(m2::RectD const & r, CoveringMode mode) : m_rect(r), m_mode(mode) {}

    IntervalsT const & Get(int scale);
  };
}

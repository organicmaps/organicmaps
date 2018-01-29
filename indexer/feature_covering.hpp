#pragma once
#include "geometry/rect2d.hpp"

#include "coding/point_to_integer.hpp"

#include <cstdint>
#include <vector>

class FeatureType;

namespace indexer
{
class LocalityObject;
}  // namespace indexer

namespace covering
{
typedef std::pair<int64_t, int64_t> Interval;
typedef std::vector<Interval> Intervals;

// Cover feature with RectIds and return their integer representations.
std::vector<int64_t> CoverFeature(FeatureType const & feature, int cellDepth,
                                  uint64_t cellPenaltyArea);

std::vector<int64_t> CoverLocality(indexer::LocalityObject const & o, int cellDepth,
                                   uint64_t cellPenaltyArea);

void AppendLowerLevels(RectId id, int cellDepth, Intervals & intervals);

// Cover viewport with RectIds and append their RectIds as well.
void CoverViewportAndAppendLowerLevels(m2::RectD const & rect, int cellDepth,
                                       Intervals & intervals);

// Given a vector of intervals [a, b), sort them and merge overlapping intervals.
Intervals SortAndMergeIntervals(Intervals const & intervals);

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
  Intervals m_res[2];

  m2::RectD const & m_rect;
  CoveringMode m_mode;

public:
  CoveringGetter(m2::RectD const & r, CoveringMode mode) : m_rect(r), m_mode(mode) {}

  Intervals const & Get(int scale);
  };
}

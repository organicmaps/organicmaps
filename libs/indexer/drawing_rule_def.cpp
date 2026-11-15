#include "indexer/drawing_rule_def.hpp"

#include <algorithm>
#include <cmath>
#include <iterator>

namespace drule
{
namespace
{
struct less_key
{
  bool operator()(drule::Key const & r1, drule::Key const & r2) const
  {
    // assume that unique algo leaves the first element (with max priority), others - go away
    if (r1.m_type == r2.m_type)
    {
      if (r1.m_hatching != r2.m_hatching)
        return r1.m_hatching;
      return (r1.m_priority > r2.m_priority);
    }
    else
      return (r1.m_type < r2.m_type);
  }
};

struct equal_key
{
  bool operator()(drule::Key const & r1, drule::Key const & r2) const
  {
    // many line rules - is ok, other rules - one is enough
    if (r1.m_type == drule::line)
      return (r1 == r2);
    else
    {
      if (r1.m_type == r2.m_type)
      {
        // Keep several area styles if bigger one (r1) is hatching and second is not.
        if (r1.m_type == drule::area && r1.m_hatching && !r2.m_hatching)
          return false;

        return true;
      }
      return false;
    }
  }
};
}  // namespace

void MakeUnique(KeysT & keys)
{
  std::sort(keys.begin(), keys.end(), less_key());
  keys.resize(std::distance(keys.begin(), std::unique(keys.begin(), keys.end(), equal_key())));
}

double CalcAreaBySizeDepth(m2::RectD const & r, int priority)
{
  ASSERT(kBasePriorityBgBySize <= priority && priority < kBasePriorityBgTop, (priority));

  // Calculate depth based on areas' bbox sizes instead of style-set priorities.
  // Raw areas' size range is about (1e-11, 3000).
  double const areaSize = r.SizeX() * r.SizeY();
  // Compacted range is approx (-37;13).
  double constexpr kMinSize = -37, kMaxSize = 13;

  // Priorities bias the size order, so that areas of similar bbox areas follow priorities, e.g.
  // landuse-forest (60 above kBasePriorityBgBySize) stays above place-islet (20) when its bbox area
  // is less than 2^(40/500) = 5.7% bigger.
  double constexpr kPrioritiesPerSizeDoubling = 500;
  double constexpr kMaxBias = kLayerPriorityRange / kPrioritiesPerSizeDoubling;
  double constexpr kStretchFactor = kDepthRangeBgBySize / (kMaxSize - kMinSize + kMaxBias);

  // Use log2() to have more precision distinguishing smaller areas.
  /// @todo We still can get here with areaSize == 0.
  double const areaSizeCompact = std::max(kMinSize, (areaSize > 0) ? std::log2(areaSize) : kMinSize);
  double const bias = (priority - kBasePriorityBgBySize) / kPrioritiesPerSizeDoubling;

  // Adjust the range to fit into [kBaseDepthBgBySize;kBaseDepthBgTop).
  double const areaDepth = kBaseDepthBgBySize + (kMaxSize - areaSizeCompact + bias) * kStretchFactor;
  ASSERT(kBaseDepthBgBySize <= areaDepth && areaDepth < kBaseDepthBgTop,
         (areaDepth, areaSize, areaSizeCompact, priority, r));

  return areaDepth;
}

}  // namespace drule

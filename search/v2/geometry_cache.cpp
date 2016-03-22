#include "search/v2/geometry_cache.hpp"

#include "search/retrieval.hpp"
#include "search/v2/mwm_context.hpp"

#include "geometry/mercator.hpp"

#include "base/assert.hpp"

#include "std/algorithm.hpp"
#include "std/utility.hpp"

namespace search
{
namespace v2
{
namespace
{
double constexpr kComparePoints = MercatorBounds::GetCellID2PointAbsEpsilon();
}  // namespace

GeometryCache::Entry::Entry() : m_scale(0) {}

GeometryCache::Entry::Entry(m2::RectD const & lowerBound, m2::RectD const & upperBound,
                            unique_ptr<coding::CompressedBitVector> cbv, int scale)
  : m_lowerBound(lowerBound), m_upperBound(upperBound), m_cbv(move(cbv)), m_scale(scale)
{
}

bool GeometryCache::Entry::Matches(m2::RectD const & rect, int scale) const
{
  return m_scale == scale && rect.IsRectInside(m_lowerBound) && m_upperBound.IsRectInside(rect);
}

GeometryCache::GeometryCache(size_t maxNumEntries, my::Cancellable const & cancellable)
  : m_maxNumEntries(maxNumEntries), m_cancellable(cancellable)
{
  CHECK_GREATER(m_maxNumEntries, 0, ());
}

coding::CompressedBitVector const * GeometryCache::Get(MwmContext const & context,
                                                       m2::RectD const & rect, int scale)
{
  auto & entries = m_entries[context.GetId()];
  auto it = entries.begin();
  for (; it != entries.end() && !it->Matches(rect, scale); ++it)
    ;
  if (it != entries.end())
  {
    if (it != entries.begin())
      iter_swap(entries.begin(), it);
    return entries.front().m_cbv.get();
  }

  auto cbv = v2::RetrieveGeometryFeatures(context, m_cancellable, rect, scale);
  entries.emplace_front(rect, m2::Inflate(rect, kComparePoints, kComparePoints), move(cbv), scale);
  if (entries.size() == m_maxNumEntries + 1)
    entries.pop_back();

  ASSERT_LESS_OR_EQUAL(entries.size(), m_maxNumEntries, ());
  ASSERT(!entries.empty(), ());

  return entries.front().m_cbv.get();
}
}  // namespace v2
}  // namespace search

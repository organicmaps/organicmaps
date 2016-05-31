#include "search/geometry_cache.hpp"

#include "search/geometry_utils.hpp"
#include "search/mwm_context.hpp"
#include "search/retrieval.hpp"

#include "geometry/mercator.hpp"

namespace search
{
namespace
{
double constexpr kCellEps = MercatorBounds::GetCellID2PointAbsEpsilon();
}  // namespace

// GeometryCache -----------------------------------------------------------------------------------
GeometryCache::GeometryCache(size_t maxNumEntries, my::Cancellable const & cancellable)
  : m_maxNumEntries(maxNumEntries), m_cancellable(cancellable)
{
  CHECK_GREATER(m_maxNumEntries, 0, ());
}

void GeometryCache::InitEntry(MwmContext const & context, m2::RectD const & rect, int scale,
                              Entry & entry)
{
  entry.m_rect = rect;
  entry.m_cbv = RetrieveGeometryFeatures(context, m_cancellable, rect, scale);
  entry.m_scale = scale;
}

// PivotRectsCache ---------------------------------------------------------------------------------
PivotRectsCache::PivotRectsCache(size_t maxNumEntries, my::Cancellable const & cancellable,
                                 double maxRadiusMeters)
  : GeometryCache(maxNumEntries, cancellable), m_maxRadiusMeters(maxRadiusMeters)
{
}

coding::CompressedBitVector const * PivotRectsCache::Get(MwmContext const & context,
                                                         m2::RectD const & rect, int scale)
{
  auto p = FindOrCreateEntry(
      context.GetId(), [&rect, &scale](Entry const & entry)
      {
        return scale == entry.m_scale &&
               (entry.m_rect.IsRectInside(rect) || IsEqualMercator(rect, entry.m_rect, kCellEps));
      });
  auto & entry = p.first;
  if (p.second)
  {
    m2::RectD normRect =
        MercatorBounds::RectByCenterXYAndSizeInMeters(rect.Center(), m_maxRadiusMeters);
    if (!normRect.IsRectInside(rect))
      normRect = rect;
    InitEntry(context, normRect, scale, entry);
  }
  return entry.m_cbv.get();
}

// LocalityRectsCache ------------------------------------------------------------------------------
LocalityRectsCache::LocalityRectsCache(size_t maxNumEntries, my::Cancellable const & cancellable)
  : GeometryCache(maxNumEntries, cancellable)
{
}

coding::CompressedBitVector const * LocalityRectsCache::Get(MwmContext const & context,
                                                            m2::RectD const & rect, int scale)
{
  auto p = FindOrCreateEntry(context.GetId(), [&rect, &scale](Entry const & entry)
                             {
                               return scale == entry.m_scale &&
                                      IsEqualMercator(rect, entry.m_rect, kCellEps);
                             });
  auto & entry = p.first;
  if (p.second)
    InitEntry(context, rect, scale, entry);
  return entry.m_cbv.get();
}

}  // namespace search

#include "map/search_viewport.hpp"

#include "indexer/feature_data.hpp"
#include "indexer/feature_utils.hpp"
#include "indexer/scales.hpp"

#include "geometry/mercator.hpp"

#include <algorithm>
#include <cmath>
#include <limits>

using namespace search;

namespace search_viewport
{
namespace
{
// Results farther than this from the viewport center (or than the viewport's half diagonal multiplied by the factor)
// are shown by moving the viewport instead of zooming it out.
double constexpr kFarDistanceMeters = 20000.0;
double constexpr kFarViewportFactor = 2.0;
// Results that fit into a rect of this scale are considered localized and are shown all at once.
int constexpr kLocalizedScale = 15;
double constexpr kMarginFactor = 0.05;

// Mercator size of a square that is shown at |scale| (a tile of the world).
// Note that this is the device-independent tile size, while the renderer's df::GetRectForDrawScale()
// additionally shifts the scale by df::GetTileScaleIncrement() = log2(tileSize / 256 / visualScale).
// They coincide on phones (the increment truncates to zero there), but on displays with a smaller
// visual scale per tile (iPad @2x, desktop) a result is shown up to two levels closer here than by
// a tap on it. Keeping the policy free of the VisualParams singleton is worth that much.
double GetSizeForScale(int scale)
{
  return mercator::Bounds::kRangeX / (1 << scale);
}

// The scale to show a single result at: its feature's own comfort scale (e.g. a city is shown farther than a cafe).
int GetScaleForResult(Result const & result)
{
  if (result.GetResultType() != Result::Type::Feature)
    return scales::GetUpperComfortScale();

  feature::TypesHolder types(feature::GeomType::Point);
  types.Assign(result.GetFeatureType());
  return feature::GetFeatureViewportScale(result.GetFeatureID(), types);
}

// Suggestions are query completions, not matches, although the ones made from features have a point.
bool IsMatchedResult(Result const & result)
{
  return result.HasPoint() && !result.IsSuggest();
}
}  // namespace

bool FitToResults(Results const & results, m2::AnyRectD & viewport)
{
  // Only the best matching results take part, e.g. a misprinted "Tribu Mins" should not outweigh "Minsk".
  uint16_t minErrors = ErrorsMade::kInfiniteErrors;
  for (auto const & r : results)
    if (IsMatchedResult(r))
      minErrors = std::min(minErrors, r.GetErrorsMade().m_errorsMade);

  m2::PointD const center = viewport.Center();
  // The viewport may be scrolled past the antimeridian: take the results in the world copy nearest to it.
  auto const nearestCopy = [&center](m2::PointD pt)
  {
    pt.x = mercator::NearestWrapX(pt.x, center.x);
    return pt;
  };

  Result const * top = nullptr;
  m2::PointD topLocalPt, nearestPt;
  double minDistance = std::numeric_limits<double>::max();
  size_t count = 0;
  // Bounding rect of the results in the local coordinates of the viewport.
  m2::RectD bounds;
  for (auto const & r : results)
  {
    if (!IsMatchedResult(r) || r.GetErrorsMade().m_errorsMade != minErrors)
      continue;

    m2::PointD const pt = nearestCopy(r.GetFeatureCenter());
    if (viewport.IsPointInside(pt))
      return false;

    m2::PointD const localPt = viewport.ConvertTo(pt);
    if (!top)
    {
      top = &r;
      topLocalPt = localPt;
    }
    ++count;
    bounds.Add(localPt);
    double const dist = center.SquaredLength(pt);
    if (dist < minDistance)
    {
      minDistance = dist;
      nearestPt = pt;
    }
  }

  if (!top)
    return false;

  m2::RectD const local = viewport.GetLocalRect();
  double const farDistance =
      std::max(kFarDistanceMeters,
               kFarViewportFactor * mercator::DistanceOnEarth(center, viewport.ConvertFrom(local.RightTop())));
  if (mercator::DistanceOnEarth(center, nearestPt) <= farDistance)
  {
    // Zoom out symmetrically around the center, keeping the original extents.
    m2::PointD const offset = viewport.ConvertTo(nearestPt) - local.Center();
    viewport.Inflate(std::max(0.0, std::fabs(offset.x) - local.SizeX() / 2),
                     std::max(0.0, std::fabs(offset.y) - local.SizeY() / 2));
  }
  else
  {
    double const localizedSize = GetSizeForScale(kLocalizedScale);
    // Too far and spread to be shown at once: show the top result only, like a tap on it.
    bool single = (count == 1);
    if (!single && (bounds.SizeX() > localizedSize || bounds.SizeY() > localizedSize))
    {
      single = true;
      bounds = m2::RectD(topLocalPt, 0.0 /* dx */, 0.0 /* dy */);
    }
    // Show all the results, but not closer than the comfort scale (or the single result's own scale).
    double const minSize = GetSizeForScale(single ? GetScaleForResult(*top) : scales::GetUpperComfortScale());
    bounds.Inflate(std::max(0.0, (minSize - bounds.SizeX()) / 2), std::max(0.0, (minSize - bounds.SizeY()) / 2));
    viewport = m2::AnyRectD(viewport.GlobalZero(), viewport.Angle(), bounds);
  }

  viewport.Inflate(viewport.GetLocalRect().SizeX() * kMarginFactor, viewport.GetLocalRect().SizeY() * kMarginFactor);
  return true;
}
}  // namespace search_viewport

#include "testing/testing.hpp"

#include "map/search_api.hpp"

#include "search/result.hpp"

#include "indexer/classificator.hpp"
#include "indexer/classificator_loader.hpp"
#include "indexer/feature_decl.hpp"
#include "indexer/mwm_set.hpp"
#include "indexer/scales.hpp"

#include "geometry/any_rect2d.hpp"
#include "geometry/mercator.hpp"
#include "geometry/point2d.hpp"
#include "geometry/rect2d.hpp"

#include <initializer_list>
#include <memory>

namespace search_viewport_tests
{
using namespace search;

double constexpr kEps = 1e-9;
// Constants of the tested policy.
double constexpr kFarDistanceMeters = 20000.0;
int constexpr kLocalizedScale = 15;

m2::PointD const kBuenosAires = mercator::FromLatLon(-34.6, -58.4);
m2::PointD const kMinsk = mercator::FromLatLon(53.9, 27.6);
m2::PointD const kTribuMins = mercator::FromLatLon(7.5, -5.5);  // Cote d'Ivoire, a misprinted match for "Minsk".

Result MakeResult(m2::PointD const & pt, uint16_t errorsMade = 0)
{
  Result res(pt, "name");
  res.SetType(Result::Type::LatLon);
  res.SetErrorsMade(ErrorsMade(errorsMade));
  return res;
}

Results MakeResults(std::initializer_list<m2::PointD> const & points)
{
  Results results;
  for (auto const & pt : points)
    results.AddResultNoChecks(MakeResult(pt));
  return results;
}

// A viewport of the given size in meters, centered at |center| and rotated by |angle|.
m2::AnyRectD MakeViewport(m2::PointD const & center, double widthMeters, double heightMeters, double angle = 0.0)
{
  double const w = mercator::MetersToMercator(widthMeters) / 2;
  double const h = mercator::MetersToMercator(heightMeters) / 2;
  return m2::AnyRectD(center, ang::AngleD(angle), m2::RectD(-w, -h, w, h));
}

// Named to avoid the ADL clash with m2::Shift().
m2::PointD ShiftMeters(m2::PointD const & pt, double dxMeters, double dyMeters)
{
  return pt + m2::PointD(mercator::MetersToMercator(dxMeters), mercator::MetersToMercator(dyMeters));
}

double SizeForScale(int scale)
{
  return mercator::Bounds::kRangeX / (1 << scale);
}

void TestUnchanged(Results const & results, m2::AnyRectD const & viewport)
{
  auto adjusted = viewport;
  TEST(!AdjustViewportToSearchResults(results, adjusted), (adjusted));
  TEST_EQUAL(adjusted.GetGlobalRect(), viewport.GetGlobalRect(), ());
}

UNIT_TEST(AdjustViewportToSearchResults_NoResults)
{
  auto const viewport = MakeViewport(kBuenosAires, 2000, 1000);
  TestUnchanged(Results(), viewport);

  Results suggests;
  suggests.AddResultNoChecks(Result("ca", "cafe"));
  TestUnchanged(suggests, viewport);
}

UNIT_TEST(AdjustViewportToSearchResults_NearestInside)
{
  auto const viewport = MakeViewport(kBuenosAires, 2000, 1000);
  // The nearest result is inside the viewport, so the others must not affect it.
  TestUnchanged(MakeResults({kMinsk, ShiftMeters(kBuenosAires, 5000, 0), ShiftMeters(kBuenosAires, 100, 100)}),
                viewport);
}

UNIT_TEST(AdjustViewportToSearchResults_NearZoomOut)
{
  for (double const angle : {0.0, math::pi / 4})
  {
    auto const original = MakeViewport(kBuenosAires, 2000, 1000, angle);
    auto viewport = original;

    m2::PointD const nearest = ShiftMeters(kBuenosAires, 200, 2000);
    m2::PointD const far = ShiftMeters(kBuenosAires, 0, 10000);
    TEST(AdjustViewportToSearchResults(MakeResults({far, nearest}), viewport), ());

    // The viewport is zoomed out around its center to include the nearest result with a margin,
    // keeping the original area and the rotation.
    TEST(viewport.IsPointInside(nearest), (viewport));
    TEST(viewport.IsRectInside(original), (viewport));
    TEST(m2::AlmostEqualAbs(viewport.Center(), original.Center(), kEps), (viewport));
    TEST_ALMOST_EQUAL_ABS(viewport.Angle().val(), angle, kEps, ());
    TEST_LESS(viewport.GetMaxSize(), mercator::MetersToMercator(3 * 2000), (viewport));
    // The viewport is not extended to include all the results.
    TEST(!viewport.IsPointInside(far), (viewport));
  }
}

UNIT_TEST(AdjustViewportToSearchResults_FarSingleResult)
{
  double const angle = math::pi / 6;
  auto viewport = MakeViewport(kBuenosAires, 2000, 1000, angle);
  TEST(AdjustViewportToSearchResults(MakeResults({kMinsk}), viewport), ());

  // The viewport is moved to the result and shows it at the comfort scale instead of being zoomed out
  // over the half of the world (which also fails to fit into the mercator bounds).
  TEST(m2::AlmostEqualAbs(viewport.Center(), kMinsk, kEps), (viewport));
  TEST(mercator::Bounds::FullRect().IsRectInside(viewport.GetGlobalRect()), (viewport));
  TEST_ALMOST_EQUAL_ABS(viewport.Angle().val(), angle, kEps, ());
  double const size = SizeForScale(scales::GetUpperComfortScale());
  TEST_GREATER_OR_EQUAL(viewport.GetLocalRect().SizeX(), size, (viewport));
  TEST_LESS(viewport.GetLocalRect().SizeX(), 2 * size, (viewport));
  TEST_ALMOST_EQUAL_ABS(viewport.GetLocalRect().SizeX(), viewport.GetLocalRect().SizeY(), kEps, (viewport));
}

UNIT_TEST(AdjustViewportToSearchResults_FarSingleCity)
{
  classificator::Load();

  auto viewport = MakeViewport(kBuenosAires, 2000, 1000);
  Results results;
  Result city(kMinsk, "Minsk");
  // A feature id of any registered country mwm.
  struct CountryMwmInfo : public MwmInfo
  {
    CountryMwmInfo()
    {
      m_minScale = 1;
      m_maxScale = scales::GetUpperScale();
      SetStatus(STATUS_REGISTERED);
    }
  };
  auto const info = std::make_shared<CountryMwmInfo>();
  city.FromFeature(FeatureID(MwmSet::MwmId(info), 0), classif().GetTypeByPath({"place", "city"}), 0 /* matchedType */,
                   {});
  results.AddResultNoChecks(std::move(city));
  TEST(AdjustViewportToSearchResults(results, viewport), ());

  // A city is shown farther than a POI.
  TEST(m2::AlmostEqualAbs(viewport.Center(), kMinsk, kEps), (viewport));
  double const size = SizeForScale(scales::GetUpperWorldScale());
  TEST_GREATER_OR_EQUAL(viewport.GetLocalRect().SizeX(), size, (viewport));
  TEST_LESS(viewport.GetLocalRect().SizeX(), 2 * size, (viewport));
}

UNIT_TEST(AdjustViewportToSearchResults_FarLocalized)
{
  double const angle = math::pi / 4;
  auto viewport = MakeViewport(kBuenosAires, 2000, 1000, angle);
  m2::PointD const p1 = ShiftMeters(kMinsk, -400, 0);
  m2::PointD const p2 = ShiftMeters(kMinsk, 300, 500);
  m2::PointD const p3 = ShiftMeters(kMinsk, 100, -200);
  TEST(AdjustViewportToSearchResults(MakeResults({p1, p2, p3}), viewport), ());

  // All the localized results are shown at once, keeping the rotation.
  for (auto const & pt : {p1, p2, p3})
    TEST(viewport.IsPointInside(pt), (viewport, pt));
  TEST_ALMOST_EQUAL_ABS(viewport.Angle().val(), angle, kEps, ());
  TEST_LESS(viewport.GetMaxSize(), SizeForScale(kLocalizedScale), (viewport));
  TEST_LESS(viewport.GetMaxSize(), mercator::MetersToMercator(2 * 1000), (viewport));
}

UNIT_TEST(AdjustViewportToSearchResults_FarSpread)
{
  double const angle = math::pi / 4;
  auto viewport = MakeViewport(kBuenosAires, 2000, 1000, angle);
  m2::PointD const nearest = ShiftMeters(kMinsk, -30000, -30000);
  TEST(AdjustViewportToSearchResults(MakeResults({kMinsk, nearest, ShiftMeters(kMinsk, 30000, 0)}), viewport), ());

  // The results are spread too much to be shown at once, so the top one is shown at its own scale
  // (not the nearest one), keeping the rotation.
  TEST(m2::AlmostEqualAbs(viewport.Center(), kMinsk, kEps), (viewport));
  double const size = SizeForScale(scales::GetUpperComfortScale());
  TEST_GREATER_OR_EQUAL(viewport.GetLocalRect().SizeX(), size, (viewport));
  TEST_LESS(viewport.GetLocalRect().SizeX(), 2 * size, (viewport));
  TEST_ALMOST_EQUAL_ABS(viewport.Angle().val(), angle, kEps, ());
  TEST(!viewport.IsPointInside(nearest), (viewport));
}

UNIT_TEST(AdjustViewportToSearchResults_MisprintsAreIgnored)
{
  // The misprinted result is much closer to the viewport, but the exact one wins.
  auto viewport = MakeViewport(kBuenosAires, 2000, 1000);
  Results results;
  results.AddResultNoChecks(MakeResult(kMinsk, 0 /* errorsMade */));
  results.AddResultNoChecks(MakeResult(kTribuMins, 1 /* errorsMade */));
  TEST(AdjustViewportToSearchResults(results, viewport), ());
  TEST(m2::AlmostEqualAbs(viewport.Center(), kMinsk, kEps), (viewport));

  // The same for the misprinted result inside the viewport: the exact one nearby is made visible.
  auto const original = MakeViewport(kBuenosAires, 2000, 1000);
  viewport = original;
  m2::PointD const exact = ShiftMeters(kBuenosAires, 0, 3000);
  results.Clear();
  results.AddResultNoChecks(MakeResult(ShiftMeters(kBuenosAires, 100, 100), 1 /* errorsMade */));
  results.AddResultNoChecks(MakeResult(exact, 0 /* errorsMade */));
  TEST(AdjustViewportToSearchResults(results, viewport), ());
  TEST(viewport.IsPointInside(exact), (viewport));
  TEST(viewport.IsRectInside(original), (viewport));

  // The best available results are used when there are no exact ones.
  viewport = original;
  results.Clear();
  results.AddResultNoChecks(MakeResult(kMinsk, 2 /* errorsMade */));
  results.AddResultNoChecks(MakeResult(kTribuMins, 1 /* errorsMade */));
  TEST(AdjustViewportToSearchResults(results, viewport), ());
  TEST(m2::AlmostEqualAbs(viewport.Center(), kTribuMins, kEps), (viewport));

  // Suggestions are query completions, not matches, and do not take part.
  viewport = original;
  results.Clear();
  Result suggestion(MakeResult(ShiftMeters(kBuenosAires, 100, 100)), "Minsk");
  results.AddResultNoChecks(std::move(suggestion));
  results.AddResultNoChecks(MakeResult(kMinsk));
  TEST(AdjustViewportToSearchResults(results, viewport), ());
  TEST(m2::AlmostEqualAbs(viewport.Center(), kMinsk, kEps), (viewport));
}

UNIT_TEST(AdjustViewportToSearchResults_Antimeridian)
{
  // The viewport is next to the antimeridian, the results are on the other side of it.
  m2::PointD const east = mercator::FromLatLon(0.0, 179.99);
  m2::PointD const west = mercator::FromLatLon(0.0, -179.99);
  auto const nearestCopy = [](m2::PointD pt, m2::PointD const & center)
  {
    pt.x = mercator::NearestWrapX(pt.x, center.x);
    return pt;
  };

  // Inside: the result is ~2.2 km away across the antimeridian.
  auto viewport = MakeViewport(east, 6000, 3000);
  TEST(!AdjustViewportToSearchResults(MakeResults({west}), viewport), (viewport));

  // Near: zoomed out around the center to the nearest world copy of the result.
  auto const original = MakeViewport(east, 1000, 500);
  viewport = original;
  TEST(AdjustViewportToSearchResults(MakeResults({west}), viewport), ());
  TEST(m2::AlmostEqualAbs(viewport.Center(), original.Center(), kEps), (viewport));
  TEST(viewport.IsPointInside(nearestCopy(west, original.Center())), (viewport));
  TEST_LESS(viewport.GetMaxSize(), mercator::MetersToMercator(10000), (viewport));

  // The map itself may be scrolled past the antimeridian (unwrapped coordinates): a far result
  // is shown in the world copy nearest to the viewport, not in the canonical one.
  m2::PointD const unwrapped = mercator::FromLatLon(0.0, 190.0);
  m2::PointD const far = mercator::FromLatLon(0.0, -160.0);  // 10 degrees to the east of the viewport.
  viewport = MakeViewport(unwrapped, 2000, 1000);
  TEST(AdjustViewportToSearchResults(MakeResults({far}), viewport), ());
  TEST(m2::AlmostEqualAbs(viewport.Center(), nearestCopy(far, unwrapped), kEps), (viewport));
  TEST_ALMOST_EQUAL_ABS(viewport.Center().x, 200.0, 1e-9, (viewport));
}

UNIT_TEST(AdjustViewportToSearchResults_FarThreshold)
{
  // Mercator meters match the real ones at the equator only.
  m2::PointD const equator = mercator::FromLatLon(0.0, 0.0);
  auto const original = MakeViewport(equator, 2000, 1000);

  // Just below the threshold: zoomed out around the center.
  auto viewport = original;
  m2::PointD const near = ShiftMeters(equator, 0, kFarDistanceMeters - 2000);
  TEST(AdjustViewportToSearchResults(MakeResults({near}), viewport), ());
  TEST(m2::AlmostEqualAbs(viewport.Center(), original.Center(), kEps), (viewport));
  TEST(viewport.IsPointInside(near), (viewport));

  // Just above the threshold: moved to the result.
  viewport = original;
  m2::PointD const far = ShiftMeters(equator, 0, kFarDistanceMeters + 2000);
  TEST(AdjustViewportToSearchResults(MakeResults({far}), viewport), ());
  TEST(m2::AlmostEqualAbs(viewport.Center(), far, kEps), (viewport));

  // The threshold grows with the viewport (30x20 km, half diagonal ~18 km), so a result just outside
  // of a big viewport is still shown by zooming out instead of a jump to the street level.
  auto const big = MakeViewport(equator, 30000, 20000);
  viewport = big;
  m2::PointD const nearForBig = ShiftMeters(equator, 0, kFarDistanceMeters + 2000);
  TEST(AdjustViewportToSearchResults(MakeResults({nearForBig}), viewport), ());
  TEST(m2::AlmostEqualAbs(viewport.Center(), big.Center(), kEps), (viewport));
  TEST(viewport.IsPointInside(nearForBig), (viewport));
  TEST(viewport.IsRectInside(big), (viewport));

  viewport = big;
  m2::PointD const farForBig = ShiftMeters(equator, 0, 40000);
  TEST(AdjustViewportToSearchResults(MakeResults({farForBig}), viewport), ());
  TEST(m2::AlmostEqualAbs(viewport.Center(), farForBig, kEps), (viewport));
}
}  // namespace search_viewport_tests

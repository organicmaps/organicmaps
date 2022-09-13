#include "testing/testing.hpp"

#include "search/search_tests_support/helpers.hpp"

#include "geometry/distance_on_sphere.hpp"

namespace real_mwm_tests
{

class MwmTestsFixture : public search::tests_support::SearchTestBase
{
public:
  // Pass LDEBUG to verbose logs for debugging.
  MwmTestsFixture() : search::tests_support::SearchTestBase(LINFO, false /* mockCountryInfo */) {}

  // Default top POIs count to check types or distances.
  static size_t constexpr kTopPoiResultsCount = 5;
  static size_t constexpr kPopularPoiResultsCount = 10;

  // Feature's centers table is created with low coordinates precision for better compression,
  // so distance-to-pivot is not precise and real meters distance may differ.
  static double constexpr kDistanceEpsilon = 5;
  static double constexpr kDefaultViewportRadiusM = 10000;
  static double constexpr kLoadMwmRadiusM = 200000;

  void SetViewportAndLoadMaps(ms::LatLon const & center, double radiusM = kDefaultViewportRadiusM)
  {
    RegisterLocalMapsInViewport(mercator::MetersToXY(center.m_lon, center.m_lat, kLoadMwmRadiusM));

    SetViewport(center, radiusM);
  }

  using ResultsT = std::vector<search::Result>;

  class Range
  {
    ResultsT const & m_v;
    size_t m_beg, m_end;

  public:
    explicit Range(ResultsT const & v) : m_v(v), m_beg(0), m_end(kTopPoiResultsCount) {}
    Range(ResultsT const & v, size_t beg, size_t end = kTopPoiResultsCount) : m_v(v), m_beg(beg), m_end(end) {}

    size_t size() const { return m_end - m_beg; }
    auto begin() const { return m_v.begin() + m_beg; }
    auto end() const { return m_v.begin() + m_end; }
    auto const & operator[](size_t i) const { return *(begin() + i); }
  };

  /// @return First (minimal) distance in meters.
  static double SortedByDistance(Range const & results, ms::LatLon const & center)
  {
    double const firstDist = ms::DistanceOnEarth(center, mercator::ToLatLon(results[0].GetFeatureCenter()));

    double prevDist = firstDist;
    for (size_t i = 1; i < results.size(); ++i)
    {
      double const dist = ms::DistanceOnEarth(center, mercator::ToLatLon(results[i].GetFeatureCenter()));
      TEST_LESS(prevDist, dist + kDistanceEpsilon, (results[i-1], results[i]));
      prevDist = dist;
    }

    return firstDist;
  }

  static std::vector<uint32_t> GetClassifTypes(std::vector<base::StringIL> const & arr)
  {
    std::vector<uint32_t> res;
    res.reserve(arr.size());

    Classificator const & c = classif();
    for (auto const & e : arr)
      res.push_back(c.GetTypeByPath(e));
    return res;
  }

  static bool EqualClassifType(uint32_t checkType, uint32_t ethalonType)
  {
    ftype::TruncValue(checkType, ftype::GetLevel(ethalonType));
    return checkType == ethalonType;
  }

  static void EqualClassifType(Range const & results, std::vector<uint32_t> const & types)
  {
    for (auto const & r : results)
    {
      auto const it = std::find_if(types.begin(), types.end(), [type = r.GetFeatureType()](uint32_t inType)
      {
        return EqualClassifType(type, inType);
      });

      TEST(it != types.end(), (r));
    }
  }

  static void NameStartsWith(Range const & results, base::StringIL const & prefixes)
  {
    for (auto const & r : results)
    {
      auto const it = std::find_if(prefixes.begin(), prefixes.end(), [name = r.GetString()](char const * prefix)
      {
        return strings::StartsWith(name, prefix);
      });

      TEST(it != prefixes.end(), (r));
    }
  }

  /// @param[in] rect { min_lon, min_lat, max_lon, max_lat }
  static void CenterInRect(Range const & results, m2::RectD const & rect)
  {
    for (auto const & r : results)
    {
      auto const ll = mercator::ToLatLon(r.GetFeatureCenter());
      TEST(rect.IsPointInside({ll.m_lon, ll.m_lat}), (r));
    }
  }

  /// @param[in] street, house May be empty.
  static void HasAddress(Range const & results, std::string const & street, std::string const & house)
  {
    auto const buildingType = classif().GetTypeByPath({"building"});

    bool found = false;
    for (auto const & r : results)
    {
      if (r.GetResultType() == search::Result::Type::Feature && EqualClassifType(r.GetFeatureType(), buildingType))
      {
        found = true;
        break;
      }
    }

    TEST(found, ());
  }

  double GetDistanceM(search::Result const & r, ms::LatLon const & ll) const
  {
    return ms::DistanceOnEarth(ll, mercator::ToLatLon(r.GetFeatureCenter()));
  }
};

// https://github.com/organicmaps/organicmaps/issues/3026
UNIT_CLASS_TEST(MwmTestsFixture, Berlin_Rewe)
{
  // Berlin
  ms::LatLon const center(52.5170365, 13.3888599);
  SetViewportAndLoadMaps(center);

  auto request = MakeRequest("rewe");
  auto const & results = request->Results();
  TEST_GREATER(results.size(), kPopularPoiResultsCount, ());

  Range const range(results, 0, kPopularPoiResultsCount);
  EqualClassifType(range, GetClassifTypes({{"shop"}, {"amenity", "fast_food"}}));
  double const dist = SortedByDistance(range, center);
  TEST_LESS(dist, 1000, ());
}

// https://github.com/organicmaps/organicmaps/issues/1376
UNIT_CLASS_TEST(MwmTestsFixture, Madrid_Carrefour)
{
  // Madrid
  ms::LatLon const center(40.41048, -3.69773);
  SetViewportAndLoadMaps(center);

  auto request = MakeRequest("carrefour");
  auto const & results = request->Results();
  TEST_GREATER(results.size(), kPopularPoiResultsCount, ());

  /// @todo 'Carrefour' city in Haiti :)
  TEST_EQUAL(results[0].GetFeatureType(), classif().GetTypeByPath({"place", "city", "capital", "3"}), ());

  Range const range(results, 1, kPopularPoiResultsCount);
  EqualClassifType(range, GetClassifTypes({{"shop"}}));
  double const dist = SortedByDistance(range, center);
  TEST_LESS(dist, 500, ());
}

// https://github.com/organicmaps/organicmaps/issues/2530
UNIT_CLASS_TEST(MwmTestsFixture, Nicosia_Jumbo)
{
  // Nicosia
  ms::LatLon const center(35.16915, 33.36141);
  SetViewportAndLoadMaps(center);

  auto request = MakeRequest("jumb");
  auto const & results = request->Results();
  TEST_GREATER(results.size(), kTopPoiResultsCount, ());

  Range const range(results);
  EqualClassifType(range, GetClassifTypes({{"shop"}, {"amenity", "parking"}}));
  double const dist = SortedByDistance(range, center);
  TEST_LESS(dist, 5000, ());
}

// https://github.com/organicmaps/organicmaps/issues/2470
UNIT_CLASS_TEST(MwmTestsFixture, Aarhus_Netto)
{
  // Aarhus
  ms::LatLon const center(56.14958, 10.20394);
  SetViewportAndLoadMaps(center);

  auto request = MakeRequest("netto");
  auto const & results = request->Results();
  TEST_GREATER(results.size(), kTopPoiResultsCount, ());

  Range const range(results);
  EqualClassifType(range, GetClassifTypes({{"shop"}}));
  double const dist = SortedByDistance(range, center);
  TEST_LESS(dist, 500, ());
}

// https://github.com/organicmaps/organicmaps/issues/2133
UNIT_CLASS_TEST(MwmTestsFixture, NY_Subway)
{
  // New York
  ms::LatLon const center(40.7355019, -73.9948155);
  SetViewportAndLoadMaps(center);

  // Interesting case, because Manhattan has high density of:
  // - "Subway" fast food
  // - railway-subway category;
  // - bus stops with name ".. subway ..";
  // + Some noname cities LIKE("Subway", 1 error) in the World.
  auto request = MakeRequest("subway");
  auto const & results = request->Results();
  TEST_GREATER(results.size(), kPopularPoiResultsCount, ());

  Range const range(results, 0, 3);
  EqualClassifType(range, GetClassifTypes({{"amenity", "fast_food"}}));
  double const dist = SortedByDistance(range, center);
  TEST_LESS(dist, 1000, ());
}

// https://github.com/organicmaps/organicmaps/issues/3249
// https://github.com/organicmaps/organicmaps/issues/1997
UNIT_CLASS_TEST(MwmTestsFixture, London_Asda)
{
  // London
  ms::LatLon const arrPivots[] = { {51.50295, 0.00325}, {51.47890,0.01062} };
  for (auto const & center : arrPivots)
  {
    SetViewportAndLoadMaps(center);

    auto request = MakeRequest("asda");
    auto const & results = request->Results();
    TEST_GREATER(results.size(), kTopPoiResultsCount, ());

    Range const range(results);
    EqualClassifType(range, GetClassifTypes({{"shop"}, {"amenity"}}));
    double const dist = SortedByDistance(range, center);
    TEST_LESS(dist, 2000, ());
  }
}

// https://github.com/organicmaps/organicmaps/issues/3103
UNIT_CLASS_TEST(MwmTestsFixture, Lyon_Aldi)
{
  // Lyon
  ms::LatLon const center(45.7578137, 4.8320114);
  SetViewportAndLoadMaps(center);

  auto request = MakeRequest("aldi");
  auto const & results = request->Results();
  TEST_GREATER(results.size(), kPopularPoiResultsCount, ());

  Range const range(results);
  EqualClassifType(range, GetClassifTypes({{"shop", "supermarket"}}));
  double const dist = SortedByDistance(range, center);
  TEST_LESS(dist, 4000, ());
}

// https://github.com/organicmaps/organicmaps/issues/1262
UNIT_CLASS_TEST(MwmTestsFixture, NY_BarnesNoble)
{
  // New York
  ms::LatLon const center(40.7355019, -73.9948155);
  SetViewportAndLoadMaps(center);

  auto request = MakeRequest("barne's & noble");
  auto const & results = request->Results();
  TEST_GREATER(results.size(), kPopularPoiResultsCount, ());

  Range const range(results);
  EqualClassifType(range, GetClassifTypes({{"shop", "books"}}));
  double const dist = SortedByDistance(range, center);
  TEST_LESS(dist, 2000, ());
}

// https://github.com/organicmaps/organicmaps/issues/2470
UNIT_CLASS_TEST(MwmTestsFixture, Hamburg_Park)
{
  // Hamburg
  ms::LatLon const center(53.5503410, 10.0006540);
  // Bremen-Munster should also be downloaded.
  SetViewportAndLoadMaps(center);

  auto request = MakeRequest("Heide-Park");
  auto const & results = request->Results();
  TEST_GREATER(results.size(), kTopPoiResultsCount, ());

  Range const range(results);
  EqualClassifType(range, GetClassifTypes(
                     {{"tourism"}, {"shop", "gift"}, {"amenity", "fast_food"}, {"highway", "bus_stop"}}));
  NameStartsWith(range, {"Heide Park", "Heide-Park"});
  double const dist = SortedByDistance(range, center);
  TEST_LESS(dist, 100000, ());
}

// https://github.com/organicmaps/organicmaps/issues/1560
UNIT_CLASS_TEST(MwmTestsFixture, Barcelona_Carrers)
{
  // Barcelona
  ms::LatLon const center(41.3828939, 2.177432);
  SetViewportAndLoadMaps(center);

  {
    auto request = MakeRequest("carrer de napols");
    auto const & results = request->Results();
    TEST_GREATER(results.size(), kTopPoiResultsCount, ());

    Range const range(results, 0, 4);
    EqualClassifType(range, GetClassifTypes({{"highway"}}));
    CenterInRect(range, {2.1651583, 41.3899995, 2.1863021, 41.4060494});
  }

  {
    auto request = MakeRequest("carrer de les planes sabadell");
    auto const & results = request->Results();
    TEST_GREATER(results.size(), kTopPoiResultsCount, ());

    Range const range(results, 0, 1);
    EqualClassifType(range, GetClassifTypes({{"highway"}}));
    CenterInRect(range, {2.1078314, 41.5437515, 2.1106129, 41.5438819});
  }
}

// https://github.com/organicmaps/organicmaps/issues/3307
// "Karlstraße" is a common street name in Germany.
UNIT_CLASS_TEST(MwmTestsFixture, Germany_Karlstraße_3)
{
  // Load all Germany.
  RegisterLocalMapsByPrefix("Germany");
  // Ulm
  SetViewport({48.40420, 9.98604}, 3000);

  auto request = MakeRequest("Karlstraße 3");
  auto const & results = request->Results();
  TEST_GREATER(results.size(), kTopPoiResultsCount, ());

  // First expected result in Ulm: https://www.openstreetmap.org/node/2293529605#map=19/48.40419/9.98615
  TEST_LESS(GetDistanceM(results[0], {48.4042014, 9.9860426}), 500, ());
}

// https://github.com/organicmaps/organicmaps/issues/3318
// https://github.com/organicmaps/organicmaps/issues/3317
UNIT_CLASS_TEST(MwmTestsFixture, IceCream)
{
  // Load all USA.
  RegisterLocalMapsByPrefix("US_");
  // Hilo, Hawaii
  ms::LatLon const center{19.7073734, -155.0815800};
  SetViewport(center, 3000);

  auto request = MakeRequest("Gelato");
  auto const & results = request->Results();
  size_t constexpr kResultsCount = 10;
  TEST_GREATER(results.size(), kResultsCount, ());

  Range const range(results, 0, kResultsCount);
  EqualClassifType(range, GetClassifTypes({{"amenity", "ice_cream"}, {"cuisine", "ice_cream"}}));
  TEST_LESS(SortedByDistance(range, center), 2000.0, ());

  auto request2 = MakeRequest("Ice cream");
  auto const & results2 = request2->Results();
  TEST_GREATER(results2.size(), kResultsCount, ());

  for (size_t i = 0; i < kResultsCount; ++i)
    TEST(results[i].GetFeatureID() == results2[i].GetFeatureID(), (results[i], results2[i]));
}

UNIT_CLASS_TEST(MwmTestsFixture, Hilo_City)
{
  // Istanbul, Kadikoy.
  ms::LatLon const center(40.98647, 29.02552);
  SetViewportAndLoadMaps(center);

  // Lets start with trailing space here.
  // Prefix search is more fuzzy and gives "Hill", "Holo", .. nearby variants.
  auto request = MakeRequest("Hilo ");
  auto const & results = request->Results();
  size_t constexpr kResultsCount = 5;  // Hilo city in Hawaii should be at the top.
  TEST_GREATER(results.size(), kResultsCount, ());

  auto const cityType = classif().GetTypeByPath({"place", "city"});

  bool found = false;
  for (size_t i = 0; i < kResultsCount; ++i)
  {
    auto const & r = results[i];
    if (r.GetResultType() == search::Result::Type::Feature &&
        r.GetString() == "Hilo" &&
        EqualClassifType(r.GetFeatureType(), cityType))
    {
      found = true;
      break;
    }
  }

  TEST(found, (results));
}

// https://github.com/organicmaps/organicmaps/issues/3266
// Moscow has suburb "Арбат" and many streets, starting from number (2-й Обыденский), (4-й Голутвинский) inside.
// So "Арбат 2" is a _well-ranked_ street result like [Suburb full match, Street full prefix],
// but we obviously expect the building "улица Арбат 2 с/1", which is a _low-ranked_ substring.
UNIT_CLASS_TEST(MwmTestsFixture, Arbat_Address)
{
  // Moscow, Arbat
  ms::LatLon const center(55.74988, 37.59240);
  SetViewportAndLoadMaps(center);

  for (auto const & query : {"Арбат 2", "Арбат 4"})
  {
    auto request = MakeRequest(query);
    auto const & results = request->Results();
    size_t constexpr kResultsCount = 3;   // Building should be at the top.
    TEST_GREATER(results.size(), kResultsCount, ());

    Range const range(results, 0, kResultsCount);
    HasAddress(range, {}, {});
  }
}

UNIT_CLASS_TEST(MwmTestsFixture, Hawaii_Address)
{
  // Honolulu
  ms::LatLon const center(21.3045470, -157.8556760);
  SetViewportAndLoadMaps(center);

  auto request = MakeRequest("1000 Ululani Street");
  auto const & results = request->Results();
  size_t constexpr kResultsCount = 3;   // Building should be at the top.
  TEST_GREATER_OR_EQUAL(results.size(), kResultsCount, ());

  Range const range(results, 0, kResultsCount);
  HasAddress(range, "Ululani Street", "1000");
}

} // namespace real_mwm_tests

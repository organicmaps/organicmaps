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
  static double constexpr kDistanceEpsilon = 10;
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
    Range(ResultsT const & v, size_t beg, size_t end = kTopPoiResultsCount) : m_v(v), m_beg(beg), m_end(end)
    {
      TEST_LESS(beg, end, ());
      TEST_GREATER_OR_EQUAL(v.size(), end, ());
    }
    explicit Range(ResultsT const & v, bool all = false)
      : Range(v, 0, all ? v.size() : kTopPoiResultsCount)
    {
    }

    size_t size() const { return m_end - m_beg; }
    auto begin() const { return m_v.begin() + m_beg; }
    auto end() const { return m_v.begin() + m_end; }
    auto const & operator[](size_t i) const { return *(begin() + i); }
  };

  /// @return [First (min), Last (max)] distance in range (meters).
  static std::pair<double, double> SortedByDistance(Range const & results, ms::LatLon const & center)
  {
    double const firstDist = GetDistanceM(results[0], center);

    double prevDist = firstDist;
    for (size_t i = 1; i < results.size(); ++i)
    {
      double const dist = GetDistanceM(results[i], center);
      TEST_LESS(prevDist, dist + kDistanceEpsilon, (results[i-1], results[i]));
      prevDist = dist;
    }

    return {firstDist, prevDist};
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

  static void EqualClassifType(Range const & results, std::vector<uint32_t> const & types)
  {
    for (auto const & r : results)
    {
      auto const it = std::find_if(types.begin(), types.end(), [&r](uint32_t inType)
      {
        return r.IsSameType(inType);
      });

      TEST(it != types.end(), (r));
    }
  }

  static size_t CountClassifType(Range const & results, uint32_t type)
  {
    return std::count_if(results.begin(), results.end(), [type](search::Result const & r)
    {
      return r.IsSameType(type);
    });
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
  static void HasAddress(Range const & results, std::string const & street, std::string const & house,
                         base::StringIL classifType = {"building"})
  {
    auto const type = classif().GetTypeByPath(classifType);

    bool found = false;
    for (auto const & r : results)
    {
      if (r.IsSameType(type))
      {
        auto const & addr = r.GetAddress();
        if ((street.empty() || addr.find(street) != std::string::npos) &&
            (house.empty() || addr.find(house) != std::string::npos))
        {
          found = true;
          break;
        }
      }
    }

    TEST(found, ());
  }

  static double GetDistanceM(search::Result const & r, ms::LatLon const & ll)
  {
    return ms::DistanceOnEarth(ll, mercator::ToLatLon(r.GetFeatureCenter()));
  }
};

UNIT_CLASS_TEST(MwmTestsFixture, TopPOIs_Smoke)
{
  // https://github.com/organicmaps/organicmaps/issues/3026
  {
    // Berlin
    ms::LatLon const center(52.5170365, 13.3888599);
    SetViewportAndLoadMaps(center);

    auto request = MakeRequest("rewe");
    auto const & results = request->Results();
    TEST_GREATER(results.size(), kPopularPoiResultsCount, ());

    Range const range(results, 0, kPopularPoiResultsCount);
    EqualClassifType(range, GetClassifTypes({{"shop"}, {"amenity", "fast_food"}}));
    TEST_LESS(SortedByDistance(range, center).first, 500, ());
  }

  // https://github.com/organicmaps/organicmaps/issues/1376
  {
    // Madrid
    ms::LatLon const center(40.41048, -3.69773);
    SetViewportAndLoadMaps(center);

    auto request = MakeRequest("carrefour");
    auto const & results = request->Results();
    TEST_GREATER(results.size(), kPopularPoiResultsCount, ());

    Range const range(results, 0, kPopularPoiResultsCount);
    EqualClassifType(range, GetClassifTypes({{"shop"}}));
    TEST_LESS(SortedByDistance(range, center).first, 200, ());
  }

  // https://github.com/organicmaps/organicmaps/issues/2530
  {
    // Nicosia
    ms::LatLon const center(35.16915, 33.36141);
    SetViewportAndLoadMaps(center);

    auto request = MakeRequest("jumb");
    auto const & results = request->Results();
    TEST_GREATER(results.size(), kTopPoiResultsCount, ());

    Range const range(results, 0, 4);
    EqualClassifType(range, GetClassifTypes({{"shop"}}));
    TEST_LESS(SortedByDistance(range, center).first, 5000, ());

    // parking (< 6km) should be on top.
    EqualClassifType(Range(results, 4, 6), GetClassifTypes({{"leisure", "playground"}, {"amenity", "parking"}}));
  }

  // https://github.com/organicmaps/organicmaps/issues/2470
  {
    // Aarhus
    ms::LatLon const center(56.14958, 10.20394);
    SetViewportAndLoadMaps(center);

    auto request = MakeRequest("netto");
    auto const & results = request->Results();
    TEST_GREATER(results.size(), kTopPoiResultsCount, ());

    Range const range(results);
    EqualClassifType(range, GetClassifTypes({{"shop"}}));
    TEST_LESS(SortedByDistance(range, center).first, 300, ());
  }
}

// https://github.com/organicmaps/organicmaps/issues/2133
UNIT_CLASS_TEST(MwmTestsFixture, NY_Subway)
{
  // New York
  ms::LatLon const center(40.7355019, -73.9948155);
  SetViewportAndLoadMaps(center);

  auto request = MakeRequest("subway");
  auto const & results = request->Results();
  TEST_GREATER(results.size(), kPopularPoiResultsCount, ());

  /// @todo First result is building-train_station with 'subway' in name;
  // - 5 nearby 'subway' fast-foods;
  // - 3 railway-station-subway;
  EqualClassifType(Range(results, 1, 6), GetClassifTypes({{"amenity", "fast_food"}}));
  EqualClassifType(Range(results, 6, 9), GetClassifTypes({{"railway", "station", "subway"}}));
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
    TEST_LESS(SortedByDistance(range, center).first, 2000, ());
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

  /// @todo First result is "L'ancre aldine", but maybe it is ok?
  Range const range(results, 1, kPopularPoiResultsCount);
  EqualClassifType(range, GetClassifTypes({{"shop", "supermarket"}}));
  TEST_LESS(SortedByDistance(range, center).first, 2000, ());
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
  TEST_LESS(SortedByDistance(range, center).first, 1000, ());
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

  Range const range(results, 0, 3);
  EqualClassifType(range, GetClassifTypes({
      {"tourism", "theme_park"},
      {"amenity", "fast_food"},
      {"shop", "gift"},
  /// @todo Add _near street_ penalty
      // {"amenity", "pharmacy"}, // "Heide-Apotheke" near the "Parkstraße" street
  }));

  NameStartsWith(range, {"Heide Park", "Heide-Park"});
  TEST_LESS(SortedByDistance(range, center).first, 100000, ());

  EqualClassifType(Range(results, 4, 7), GetClassifTypes({
      {"highway", "service"},
      {"railway", "halt"},
      {"highway", "bus_stop"},
  }));
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

    // - First 2 streets in Barcelona (1-2 km)
    // - Next streets in Badalona, Barbera, Sadabel, ... (20-30 km)
    // - Again 2 _minor_ footways in Barcelona (1-2 km)
    Range const range(results, 0, 2);
    EqualClassifType(range, GetClassifTypes({{"highway"}}));
    CenterInRect(range, {2.1651583, 41.3899995, 2.1863021, 41.4060494});
  }

  // In case of a city, distance rank should be from it's origin.
  {
    auto request = MakeRequest("carrer de les planes sabadell");
    auto const & results = request->Results();
    TEST_GREATER(results.size(), kTopPoiResultsCount, ());

    Range const range(results, 0, 1);
    EqualClassifType(range, GetClassifTypes({{"highway"}}));
    CenterInRect(range, {2.1078314, 41.5437515, 2.1106129, 41.5438819});
  }

  {
    // Прилукская Слобода, Минск
    ms::LatLon const center(53.8197647, 27.4701662);
    SetViewportAndLoadMaps(center);

    auto request = MakeRequest("минск малая ул", "ru");
    auto const & results = request->Results();
    TEST_GREATER(results.size(), kTopPoiResultsCount, ());

    EqualClassifType(Range(results, 0, 1), GetClassifTypes({{"highway"}}));
    CenterInRect(Range(results, 0, 1), {27.5134786, 53.8717921, 27.5210173, 53.875768});

    /// @todo Second result is expected to be the nearby street in "Прилукская Слобода", but not always ..
    //TEST_GREATER(GetDistanceM(results[0], center), GetDistanceM(results[1], center), ());
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
  size_t constexpr kResultsCount = kPopularPoiResultsCount;
  TEST_GREATER(results.size(), kResultsCount, ());

  Range const range(results, 0, kResultsCount);
  EqualClassifType(range, GetClassifTypes({{"amenity", "ice_cream"}, {"cuisine", "ice_cream"}}));
  TEST_LESS(SortedByDistance(range, center).first, 2000.0, ());

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
    if (r.GetString() == "Hilo" && r.IsSameType(cityType))
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
    // Address should be at the top.
    HasAddress(Range(request->Results(), 0, 3), {}, {});
  }
}

UNIT_CLASS_TEST(MwmTestsFixture, Hawaii_Address)
{
  // Honolulu
  ms::LatLon const center(21.3045470, -157.8556760);
  SetViewportAndLoadMaps(center);

  auto request = MakeRequest("1000 Ululani Street");
  // Address should be at the top.
  HasAddress(Range(request->Results(), 0, 3), "Ululani Street", "1000", {"entrance"});
}

// https://github.com/organicmaps/organicmaps/issues/3712
UNIT_CLASS_TEST(MwmTestsFixture, French_StopWord_Category)
{
  // Metz
  ms::LatLon const center(49.12163, 6.17075);
  SetViewportAndLoadMaps(center);

  auto request = MakeRequest("Bouche d'incendie", "fr");
  auto const & results = request->Results();

  Range const range(results);
  EqualClassifType(range, GetClassifTypes({{"emergency", "fire_hydrant"}}));
  TEST_LESS(SortedByDistance(range, center).first, 1000.0, ());
}

UNIT_CLASS_TEST(MwmTestsFixture, Street_BusStop)
{
  // Buenos Aires
  // Also should download Argentina_Santa Fe.
  ms::LatLon const center(-34.60655, -58.43566);
  SetViewportAndLoadMaps(center);

  {
    auto request = MakeRequest("Juncal", "en");
    auto const & results = request->Results();
    TEST_GREATER(results.size(), kPopularPoiResultsCount, ());

    // Top results are Amenities, Hotels, Shops and Streets.
    // Full Match street (20 km) is better than Full Prefix bus stop (1 km).
    Range const range(results);
    EqualClassifType(range, GetClassifTypes({{"tourism", "hotel"}, {"shop"}, {"amenity"}, {"highway", "residential"}}));
  }

  {
    auto request = MakeRequest("Juncal st ", "en");
    auto const & results = request->Results();
    TEST_GREATER(results.size(), kTopPoiResultsCount, ());

    // Top results are Streets (maybe in different cities).
    Range const range(results);
    EqualClassifType(range, GetClassifTypes({{"highway"}}));
    TEST_LESS(SortedByDistance(range, center).first, 5000.0, ());
  }

  {
    auto request = MakeRequest("Juncal bus ", "en");
    auto const & results = request->Results();
    TEST_GREATER(results.size(), kTopPoiResultsCount, ());

    // Top results are bus stops.
    Range const range(results);
    EqualClassifType(range, GetClassifTypes({{"highway", "bus_stop"}}));
    TEST_LESS(SortedByDistance(range, center).first, 5000.0, ());
  }

  /// @todo Actually, we have very fancy matching here, starting from 3rd result and below.
  /// Interesting to check how it happens.
  {
    auto request = MakeRequest("Juncal train", "en");
    auto const & results = request->Results();
    TEST_GREATER(results.size(), kTopPoiResultsCount, ());

    // -1 "Juncal" supermarket near the train station, 24km :)
    // -2 Train station building in "Juncal" village, other MWM, >200km
    // -3 Train station building near "Juncal" street, 28km
    // -4 Railway station, same as (3)
    // -5 Railway station, same as (2)
    Range const range(results);
    EqualClassifType(range, GetClassifTypes({{"shop", "supermarket"},
                                             {"railway", "station"}, {"building", "train_station"}}));
    TEST_LESS(SortedByDistance(Range(results, 0, 2), center).first, 23.0E3, ());
  }
}

UNIT_CLASS_TEST(MwmTestsFixture, Generic_Buildings_Rank)
{
  // Buenos Aires
  ms::LatLon const center(-34.60655, -58.43566);
  SetViewportAndLoadMaps(center);

  {
    size_t constexpr kResultsCount = kPopularPoiResultsCount;
    auto request = MakeRequest("cell tower", "en");
    auto const & results = request->Results();
    TEST_GREATER(results.size(), kResultsCount, ());

    // results[0] is a named POI ~9km (https://www.openstreetmap.org/node/9730886727)
    Range const range(results, 1, kResultsCount);
    EqualClassifType(range, GetClassifTypes({{"man_made", "tower", "communication"}}));
    TEST_LESS(SortedByDistance(range, center).first, 3000.0, ());
  }

  {
    auto request = MakeRequest("dia ", "en");
    auto const & results = request->Results();
    TEST_GREATER(results.size(), kTopPoiResultsCount, ());

    Range const range(results);
    EqualClassifType(range, GetClassifTypes({{"shop", "supermarket"}}));
    TEST_LESS(SortedByDistance(range, center).second, 1000.0, ());
  }
}

UNIT_CLASS_TEST(MwmTestsFixture, UTH_Airport)
{
  auto const aeroportType = classif().GetTypeByPath({"aeroway", "aerodrome", "international"});

  // Under UTH airport.
  ms::LatLon const center(17.3867863, 102.7775625);
  SetViewportAndLoadMaps(center);

  // "ut" query is _common_
  auto request = MakeRequest("ut", "en");
  auto const & results = request->Results();

  bool found = false;
  // The first 5 will be cities suggestions.
  for (size_t i = 0; i < 10; ++i)
  {
    auto const & r = results[i];
    if (r.IsSameType(aeroportType))
    {
      found = true;
      break;
    }
  }

  TEST(found, (results));
}

// https://github.com/organicmaps/organicmaps/issues/5186
UNIT_CLASS_TEST(MwmTestsFixture, Milan_Streets)
{
  // Milan
  ms::LatLon const center(45.46411, 9.19045);
  SetViewportAndLoadMaps(center);

  auto request = MakeRequest("Via Domenichino", "it");
  auto const & results = request->Results();

  size_t constexpr kResultsCount = 2;
  TEST_GREATER(results.size(), kResultsCount, ());

  Range const range(results, 0, kResultsCount);
  TEST_LESS(SortedByDistance(range, center).second, 20000.0, ());
}

// https://github.com/organicmaps/organicmaps/issues/5150
UNIT_CLASS_TEST(MwmTestsFixture, London_RedLion)
{
  // Milan
  ms::LatLon const center(51.49263, -0.12877);
  SetViewportAndLoadMaps(center);

  auto request = MakeRequest("Red Lion", "en");
  auto const & results = request->Results();

  TEST_GREATER(results.size(), kPopularPoiResultsCount, ());

  // Top first results "The Red Lion" in 5 km.
  Range const range(results);
  TEST_LESS(SortedByDistance(range, center).first, 5000.0, ());
}

UNIT_CLASS_TEST(MwmTestsFixture, AddrInterpolation_Rank)
{
  // Buenos Aires (Palermo)
  ms::LatLon const center(-34.57852, -58.42567);
  SetViewportAndLoadMaps(center);

  {
    auto request = MakeRequest("Sante Fe 1176", "en");
    auto const & results = request->Results();

    TEST_GREATER(results.size(), kPopularPoiResultsCount, ());

    // Top first address results in 15 km.
    Range const range(results, 0, 2);
    EqualClassifType(range, GetClassifTypes({{"addr:interpolation"}}));
    TEST_LESS(SortedByDistance(range, center).second, 15000.0, ());

    // - 3(4) place: Exact address in Montevideo, Uruguay (~200km)
    // - 4+ places: addr:interpolation in Argentina
  }

  // Funny object here. barrier=fence with address and postcode=2700.
  // We should rank it lower than housenumber matchings. Or not?
  // https://www.openstreetmap.org/way/582640784#map=19/-33.91495/-60.55215
  {
    auto request = MakeRequest("José Hernández 2700", "en");
    auto const & results = request->Results();

    TEST_GREATER(results.size(), kPopularPoiResultsCount, ());

    // Top first address results.
    EqualClassifType(Range(results, 0, 4), GetClassifTypes({{"building", "address"}}));
  }
}

// The idea behind that is to get most famous cities on first (rare second) place.
// Every city has POIs, Stops (Metro), Streets named like other cities, but anyway - city should be on top.
// Each city should have population rank and popularity (will be implemented) that gives them big search rank.
/// @todo Add (restore) popularity at least from Wiki size.
/// @todo Add more search query languages?
UNIT_CLASS_TEST(MwmTestsFixture, Famous_Cities_Rank)
{
  auto const & cl = classif();
  uint32_t const capitalType = cl.GetTypeByPath({"place", "city", "capital"});
  uint32_t const cityType = cl.GetTypeByPath({"place", "city"});

  std::string arrCities[] = {
      "Buenos Aires",
      "Rio de Janeiro",
      "New York",
      "San Francisco",  // not a capital
      "Las Vegas",      // not a capital
      "Los Angeles",
      "Toronto",
      "Lisboa",
      "Madrid",
      "Barcelona",
      "London",
      "Paris",
      "Zurich",         // not a capital
      "Rome",
      "Milan",
      "Venezia",
      "Amsterdam",
      "Berlin",
      "Stockholm",
      "Istanbul",
      "Minsk",
      "Moscow",
      "Kyiv",
      "New Delhi",
      "Bangkok",
      "Beijing",
      "Tokyo",
      "Melbourne",
      "Sydney",
  };
  size_t const count = std::size(arrCities);

  std::set<std::string> const notCapital = { "San Francisco", "Las Vegas", "Zurich" };

  std::vector<ms::LatLon> arrCenters(count);
  // Buenos Aires like starting point :)
  arrCenters[0] = {-34.60649, -58.43540};

  size_t errorsNum = 0;

  // For DEBUG.
  // bool isGoGo = false;
  for (size_t i = 0; i < count; ++i)
  {
    // if (!isGoGo && arrCities[i] == "London")
    //   isGoGo = true;
    // if (i > 0 && !isGoGo)
    //   continue;

    size_t constexpr kTopNumber = 3;

    LOG(LINFO, ("=== Processing:", arrCities[i]));
    SetViewportAndLoadMaps(arrCenters[i]);

    for (size_t j = 0; j < count; ++j)
    {
      auto request = MakeRequest(arrCities[j] + " ", "en");
      auto const & results = request->Results();
      TEST(!results.empty(), (arrCities[i], arrCities[j]));

      // Check that needed city is in top.
      auto const iEnd = results.begin() + std::min(kTopNumber, results.size());
      auto const it = std::find_if(results.begin(), iEnd, [&](search::Result const & r)
      {
        // Should make this complex check to ensure that we got a needed city.
        uint32_t type = r.GetFeatureType();
        if (notCapital.count(arrCities[j]) > 0)
        {
          ftype::TruncValue(type, 2);
          return (type == cityType);
        }
        else
        {
          ftype::TruncValue(type, 3);
          return (type == capitalType);
        }
      });

      if (it == iEnd)
      {
        LOG(LWARNING, ("Not matched:", arrCities[i], ":", arrCities[j]));
        ++errorsNum;
      }
      else
      {
        // Fill centers table while processing first city.
        if (!arrCenters[j].IsValid())
          arrCenters[j] = mercator::ToLatLon(it->GetFeatureCenter());
      }
    }
  }

  TEST_LESS(errorsNum, 10, ());
}

UNIT_CLASS_TEST(MwmTestsFixture, Conscription_HN)
{
  // Brno (Czech)
  ms::LatLon const center(49.19217, 16.61121);
  SetViewportAndLoadMaps(center);

  for (std::string hn : {"77", "29"})
  {
    // postcode + street + house number
    auto request = MakeRequest("63900 Havlenova " + hn, "cs");
    // Should be the first result.
    HasAddress(Range(request->Results(), 0, 1), "Havlenova", "77/29");
  }
}

UNIT_CLASS_TEST(MwmTestsFixture, ToiletAirport)
{
  // Frankfurt Airport
  ms::LatLon const center(50.052108, 8.571086);
  SetViewportAndLoadMaps(center);

  for (bool const isCategory : {false, true})
  {
    auto params = GetDefaultSearchParams("toilet");
    params.m_categorialRequest = isCategory;
    size_t constexpr kResultsCount = 30;
    TEST_GREATER_OR_EQUAL(params.m_maxNumResults, kResultsCount, ());

    auto request = MakeRequest(params);
    auto const & results = request->Results();
    TEST_EQUAL(results.size(), kResultsCount, ());

    Range const range(results, 0, kResultsCount);
    EqualClassifType(range, GetClassifTypes({{"amenity", "toilets"}, {"toilets", "yes"}}));
    TEST_LESS(SortedByDistance(range, center).second, 1000, ());
  }
}

UNIT_CLASS_TEST(MwmTestsFixture, WaterTap)
{
  // UK, Christchurch
  ms::LatLon const center(50.744914, -1.787959);
  SetViewportAndLoadMaps(center);

  for (bool const isCategory : {false, true})
  {
    auto params = GetDefaultSearchParams("water tap ");
    params.m_categorialRequest = isCategory;

    auto request = MakeRequest(params);
    auto const & results = request->Results();
    TEST_GREATER_OR_EQUAL(results.size(), kTopPoiResultsCount, ());

    Range const range(results);
    EqualClassifType(range, GetClassifTypes({{"amenity", "drinking_water"}, {"man_made", "water_tap"}}));
    TEST_LESS(SortedByDistance(range, center).second, 3500, ());
  }
}

UNIT_CLASS_TEST(MwmTestsFixture, BA_LasHeras)
{
  // Buenos Aires (Palermo)
  ms::LatLon const center(-34.5801125, -58.4158058);
  SetViewportAndLoadMaps(center);

  {
    auto request = MakeRequest("Las Heras 2900");
    auto const & results = request->Results();
    TEST_GREATER(results.size(), kPopularPoiResultsCount, ());

    // First results will be:
    // - _exact_ street match addresses "Las Heras 2900"
    // - "Las Heras 2900" bus stop (only one, avoid a bunch of duplicates)

    // _sub-string_ street address match
    HasAddress(Range(results, 0, 5), "Avenida General Las Heras", "2900");
  }
}

UNIT_CLASS_TEST(MwmTestsFixture, BA_SanMartin)
{
  // Buenos Aires (Palermo)
  ms::LatLon const center(-34.5801392, -58.415764);
  SetViewportAndLoadMaps(center);

  {
    /// @todo
    /// - "Parque San Martin", "General San Martin" towns should be on top
    /// - Railway station sometimes fail because of "shuffle". Can increase m_everywhereBatchSize var.

    auto request = MakeRequest("San Martin");
    auto const & results = request->Results();
    size_t constexpr kResultsCount = 12;
    TEST_GREATER(results.size(), kResultsCount, ());
    TEST_GREATER(CountClassifType(Range(results, 0, kResultsCount),
                                  classif().GetTypeByPath({"railway", "station"})), 2, ());
  }
}

UNIT_CLASS_TEST(MwmTestsFixture, Full_Address)
{
  /* Hm, these addreses have been changed.
   * https://www.openstreetmap.org/#map=19/49.73635/19.59212
  {
    // Krakow
    ms::LatLon const center(50.061431, 19.9361584);
    SetViewportAndLoadMaps(center);

    auto request = MakeRequest("Sucha Beskidzka Armii Krajowej b-1 kozikowka  34-200 Poland");
    auto const & results = request->Results();
    TEST_GREATER(results.size(), kPopularPoiResultsCount, ());

    HasAddress(Range(results, 0, 3), "Armii Krajowej", "B-1");
    HasAddress(Range(results, 0, 3), "Armii Krajowej", "B-1A");
  }
  */

  {
    // Regensburg (DE)
    ms::LatLon const center(49.0195332, 12.0974856);
    SetViewportAndLoadMaps(center);

    /// @todo There is a tricky neighborhood here so ranking gets dumb (and we treat 'A' as a stopword).
    /// Anyway, needed addresses are in top 3 among:
    /// -1 "Gewerbepark A", "A 1"
    /// -2 "Gewerbepark B", "1"
    /// -3 "Gewerbepark C", "1"

    {
      auto request = MakeRequest("Wörth an der Donau Gewerbepark A 1 93086 Germany");
      auto const & results = request->Results();
      TEST_GREATER(results.size(), kPopularPoiResultsCount, ());

      HasAddress(Range(results, 0, 3), "Gewerbepark A", "A 1", {"shop", "car"});
    }
    {
      auto request = MakeRequest("Wörth an der Donau Gewerbepark C 1 93086 Germany");
      auto const & results = request->Results();
      TEST_GREATER(results.size(), kPopularPoiResultsCount, ());

      HasAddress(Range(results, 0, 3), "Gewerbepark C", "1");
    }
  }
}

UNIT_CLASS_TEST(MwmTestsFixture, RelaxedStreets)
{
  {
    // Buenos Aires (Palermo)
    ms::LatLon const center(-34.5802699, -58.4124979);
    SetViewportAndLoadMaps(center);

    auto request = MakeRequest("French 1700");
    auto const & results = request->Results();
    TEST_GREATER(results.size(), 20, ());

    uint32_t const building = classif().GetTypeByPath({"building", "address"});
    size_t count = 0;
    for (auto const & r : results)
    {
      if (r.GetFeatureType() != building)
      {
        TEST_EQUAL(classif().GetTypeByPath({"highway", "residential"}), r.GetFeatureType(), ());
        // First street after addresses  should be the closest one.
        TEST_LESS(GetDistanceM(r, center), 1000, ());
        break;
      }
      ++count;
    }

    // "Buenos Aires" neighbour regions should present to get >5 addresses.
    TEST_GREATER(count, 5, ());
  }

  {
    // Molodechno
    ms::LatLon const center(54.3021037, 26.8366091);
    SetViewportAndLoadMaps(center);

    auto request = MakeRequest("Молодечно первомайская", "ru");
    auto const & results = request->Results();
    TEST_GREATER(results.size(), 20, ());

    Range const range(results, 0, 3);
    EqualClassifType(range, GetClassifTypes({{"highway", "residential"}}));
    TEST_LESS(SortedByDistance(range, center).first, 3000, ());
  }
}

// https://github.com/organicmaps/organicmaps/issues/6127
UNIT_CLASS_TEST(MwmTestsFixture, Streets_Rank)
{
  {
    // Buenos Aires (Palermo)
    ms::LatLon const center(-34.5802699, -58.4124979);
    SetViewportAndLoadMaps(center);

    auto const & streetChecker = ftypes::IsStreetOrSquareChecker::Instance();

    auto const processRequest = [&](std::string const & query, size_t idx)
    {
      auto request = MakeRequest(query);
      auto const & results = request->Results();

      TEST_GREATER(results.size(), idx, ());

      bool found = false;
      for (size_t i = 0; i < idx && !found; ++i)
      {
        auto const & r = results[i];
        if (streetChecker(r.GetFeatureType()))
        {
          TEST_EQUAL(r.GetString(), "Avenida Santa Fe", ());
          TEST_LESS(GetDistanceM(r, center), 1000, ());
          found = true;
        }
      }
      TEST(found, ());
    };

    /// @todo Streets should be highwer. Now POIs additional rank is greater than Streets rank.
    processRequest("Santa Fe ", 19);
    /// @todo Prefix search gives POIs (Starbucks) near "Avenida Santa Fe". Some WTFs for street's ranking here:
    /// - gives 'Full Prefix' name's rank
    /// - gives m_matchedFraction: 0.777778, while 'st' should be counted for streets definitely
    processRequest("Santa Fe st ", 12);
  }

  {
    // Load all Hungary.
    RegisterLocalMapsByPrefix("Hungary");
    // Budapest
    SetViewport({47.4978839, 19.0401458}, 3000);

    auto request = MakeRequest("Szank Beke");
    auto const & results = request->Results();

    size_t constexpr kResultsCount = 4;
    TEST_GREATER(results.size(), kResultsCount, ());
    // - "Béke utca" in Szank
    // - "Szani Gyros" fast food near the "Béke utca"
    // - "Béke utca"
    EqualClassifType(Range(results, 0, kResultsCount), GetClassifTypes({{"highway"}, {"amenity", "fast_food"}}));
  }

  {
    // Minsk
    ms::LatLon const center(53.91058, 27.54519);
    SetViewportAndLoadMaps(center);
    auto request = MakeRequest("победител", "ru");
    auto const & results = request->Results();
    TEST_GREATER(results.size(), kTopPoiResultsCount, ());

    // First result is a viewpoint (attraction).
    EqualClassifType(Range(results, 0, 1), GetClassifTypes({{"tourism", "viewpoint"}}));
    // And next are the streets.
    Range const range(results, 1, 4);
    EqualClassifType(range, GetClassifTypes({{"highway"}}));
    TEST_LESS(SortedByDistance(range, center).first, 500, ());
  }
}

UNIT_CLASS_TEST(MwmTestsFixture, Pois_Rank)
{
  {
    // Buenos Aires (Palermo)
    ms::LatLon const center(-34.58524, -58.42516);
    SetViewportAndLoadMaps(center);

    {
      // mix of amenity=pharmacy + shop=chemistry.
      auto request = MakeRequest("farmacity");
      auto const & results = request->Results();
      TEST_GREATER(results.size(), kPopularPoiResultsCount, ());
      TEST_LESS(SortedByDistance(Range(results, 0, kPopularPoiResultsCount), center).second, 2000, ());
    }

    {
      auto request = MakeRequest("Malabia");
      auto const & results = request->Results();
      TEST_GREATER(results.size(), kPopularPoiResultsCount, ());

      // alt_name="Malabia"
      // name="Malabia XXX" - should take this name's rank
      EqualClassifType(Range(results, 0, 1), GetClassifTypes({{"railway", "station", "subway"}}));
    }
  }

  {
    // Salem, Oregon
    ms::LatLon const center(44.8856466, -123.0628986);
    SetViewportAndLoadMaps(center);

    auto request = MakeRequest("depot");
    auto const & results = request->Results();

    // - nearest post office "Mail Depot" (500m)
    // - railway station "XXX Depot"
    // - a bunch of streets and POIs "Depot XXX" (up to 100km)
    size_t constexpr kResultsCount = 20;
    TEST_GREATER(results.size(), kResultsCount, ());

    Range range(results, 0, 2);
    EqualClassifType(range, GetClassifTypes({{"amenity", "post_office"}, {"railway", "station"}}));
    TEST_LESS(SortedByDistance(range, center).first, 1000, ());
  }

  // https://github.com/organicmaps/organicmaps/issues/1017
  {
    // Adazi, Latvia
    ms::LatLon const center(57.0819824, 24.3243274);
    SetViewportAndLoadMaps(center);

    auto request = MakeRequest("rimi");
    auto const & results = request->Results();

    size_t constexpr kResultsCount = 20;
    TEST_GREATER(results.size(), kResultsCount, ());

    // result[0] - 'Rimini' city in Italy
    // First 2 results - nearest supermarkets.
    Range range(results, 1, 3);
    EqualClassifType(range, GetClassifTypes({{"shop", "supermarket"}}));
    TEST_LESS(SortedByDistance(range, center).second, 1500, ());
  }

  // https://github.com/organicmaps/organicmaps/issues/5756
  {
    // Istanbul
    ms::LatLon const center(40.95058, 29.17255);
    SetViewportAndLoadMaps(center);

    auto request = MakeRequest("Göztepe 60. Yıl Parkı");
    auto const & results = request->Results();
    TEST_GREATER(results.size(), kTopPoiResultsCount, ());
    EqualClassifType(Range(results, 0, 1), GetClassifTypes({{"leisure", "park"}}));
  }

  // https://github.com/organicmaps/organicmaps/issues/4291
  {
    // Greenville, SC
    ms::LatLon const center(34.8513533, -82.3984875);
    SetViewportAndLoadMaps(center);

    auto request = MakeRequest("Greenville Hospital");
    auto const & results = request->Results();
    TEST_GREATER(results.size(), kTopPoiResultsCount, ());

    // - First is an airport.
    // - Bus stops in the middle.
    uint32_t const hospitalType = classif().GetTypeByPath({"amenity", "hospital"});
    size_t count = 0;
    for (size_t i = 0; i < kTopPoiResultsCount; ++i)
    {
      if (results[i].GetFeatureType() == hospitalType)
      {
        ++count;
        TEST(results[i].GetString().find("Greenville") != std::string::npos, ());
      }
    }
    TEST_GREATER_OR_EQUAL(count, 2, ());
  }
}

// "San Francisco" is always an interesting query, because in Latin America there are:
// - hundreds of cities with similar names,
// - thousands of streets/POIs
UNIT_CLASS_TEST(MwmTestsFixture, San_Francisco)
{
  auto const & cl = classif();
  {
    // New York
    ms::LatLon const center(40.71253, -74.00628);
    SetViewportAndLoadMaps(center);

    auto request = MakeRequest("San Francisco");
    auto const & results = request->Results();
    TEST_GREATER(results.size(), kTopPoiResultsCount, ());

    TEST_EQUAL(results[0].GetFeatureType(), cl.GetTypeByPath({"shop", "laundry"}), ());
    TEST_LESS(GetDistanceM(results[0], center), 1.0E4, ());

    TEST_EQUAL(results[1].GetFeatureType(), cl.GetTypeByPath({"place", "city"}), ());
    TEST_LESS(GetDistanceM(results[1], center), 4.2E6, ());
  }
}

UNIT_CLASS_TEST(MwmTestsFixture, Opfikon_Viewport)
{
  auto const & cl = classif();

  // Opfikon, Zurich
  ms::LatLon const center(47.42404, 8.5667463);
  SetViewportAndLoadMaps(center);

  auto params = GetDefaultSearchParams("ofikon");     // spelling error
  params.m_mode = search::Mode::Viewport;
  params.m_maxNumResults = search::SearchParams::kDefaultNumResultsInViewport;

  {
    params.m_viewport = { 8.5418196173686862238, 53.987953174041166449, 8.5900051973703153152, 54.022951994627547379 };

    auto request = MakeRequest(params);
    auto const & results = request->Results();
    TEST_GREATER(results.size(), kPopularPoiResultsCount, ());

    Range allRange(results, true /* all */);
    // Top result by linear rank.
    TEST_EQUAL(CountClassifType(allRange, cl.GetTypeByPath({"place", "town"})), 1, ());
    // Important to check that we don't show alt_name matching in viewport.
    TEST_EQUAL(CountClassifType(allRange, cl.GetTypeByPath({"shop"})), 1, ());

    // Results with lowest rank.
    TEST_EQUAL(CountClassifType(allRange, cl.GetTypeByPath({"barrier"})), 1, ());
    TEST_EQUAL(CountClassifType(allRange, cl.GetTypeByPath({"power", "substation"})), 1, ());

    // They are exist if there are no displacement (m_minDistanceOnMapBetweenResults == {0,0})
    TEST_EQUAL(CountClassifType(allRange, cl.GetTypeByPath({"tourism"})), 2, ());

    TEST_EQUAL(CountClassifType(allRange, cl.GetTypeByPath({"highway", "bus_stop"})), 3, ());
    TEST_EQUAL(CountClassifType(allRange, cl.GetTypeByPath({"leisure"})), 3, ());

    TEST_GREATER(CountClassifType(allRange, cl.GetTypeByPath({"highway"})), 8, ());
    TEST_GREATER(CountClassifType(allRange, cl.GetTypeByPath({"amenity"})), 12, ());
  }
}

} // namespace real_mwm_tests

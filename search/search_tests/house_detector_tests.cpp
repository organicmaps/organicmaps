#include "testing/testing.hpp"

#include "search/house_detector.hpp"

#include "indexer/classificator_loader.hpp"
#include "indexer/data_header.hpp"
#include "indexer/data_source.hpp"
#include "indexer/ftypes_matcher.hpp"
#include "indexer/scales.hpp"
#include "indexer/search_string_utils.hpp"

#include "platform/local_country_file.hpp"
#include "platform/local_country_file_utils.hpp"
#include "platform/platform.hpp"

#include "geometry/distance_on_sphere.hpp"

#include "base/logging.hpp"

#include "std/iostream.hpp"
#include "std/fstream.hpp"


using platform::LocalCountryFile;

class StreetIDsByName
{
  vector<FeatureID> vect;

public:
  vector<string> streetNames;

  void operator()(FeatureType & f)
  {
    if (f.GetFeatureType() == feature::GEOM_LINE)
    {
      string name;
      if (f.GetName(0, name) &&
          find(streetNames.begin(), streetNames.end(), name) != streetNames.end())
      {
        vect.push_back(f.GetID());
      }
    }
  }

  size_t GetCount() const { return vect.size(); }

  vector<FeatureID> const & GetFeatureIDs()
  {
    sort(vect.begin(), vect.end());
    return vect;
  }
};

class CollectStreetIDs
{
  static bool GetKey(string const & name, string & key)
  {
    TEST(!name.empty(), ());
    key = strings::ToUtf8(search::GetStreetNameAsKey(name));

    if (key.empty())
    {
      LOG(LWARNING, ("Empty street key for name", name));
      return false;
    }
    return true;
  }

  typedef map<string, vector<FeatureID> > ContT;
  ContT m_ids;
  vector<FeatureID> m_empty;

public:
  void operator()(FeatureType & f)
  {
    if (f.GetFeatureType() == feature::GEOM_LINE)
    {
      string name;
      if (f.GetName(0, name) && ftypes::IsStreetChecker::Instance()(f))
      {
        string key;
        if (GetKey(name, key))
          m_ids[key].push_back(f.GetID());
      }
    }
  }

  void Finish()
  {
    for (ContT::iterator i = m_ids.begin(); i != m_ids.end(); ++i)
      sort(i->second.begin(), i->second.end());
  }

  vector<FeatureID> const & Get(string const & key) const
  {
    ContT::const_iterator i = m_ids.find(key);
    return (i == m_ids.end() ? m_empty : i->second);
  }
};

UNIT_TEST(HS_ParseNumber)
{
  typedef search::ParsedNumber NumberT;

  {
    NumberT n("135");
    TEST(n.IsOdd(), ());
    TEST_EQUAL(n.GetIntNumber(), 135, ());

    NumberT n1("133");
    NumberT n2("137");
    TEST(n.IsIntersect(n1, 2), ());
    TEST(!n.IsIntersect(n1, 1), ());
    TEST(n.IsIntersect(n2, 2), ());
    TEST(!n.IsIntersect(n2, 1), ());
  }

  {
    NumberT n("135 1к/2");
    TEST(n.IsOdd(), ());
    TEST_EQUAL(n.GetIntNumber(), 135, ());

    TEST(!n.IsIntersect(NumberT("134")), ());
    TEST(!n.IsIntersect(NumberT("136")), ());
  }

  {
    NumberT n("135A");
    TEST(n.IsOdd(), ());
    TEST_EQUAL(n.GetIntNumber(), 135, ());

    TEST(!n.IsIntersect(NumberT("134")), ());
    TEST(!n.IsIntersect(NumberT("136")), ());
  }

  {
    NumberT n("135-к1", false);
    TEST(n.IsOdd(), ());
    TEST_EQUAL(n.GetIntNumber(), 135, ());

    TEST(!n.IsIntersect(NumberT("134")), ());
    TEST(!n.IsIntersect(NumberT("136")), ());
  }

  {
    NumberT n("135-12", false);
    TEST(n.IsOdd(), ());
    TEST_EQUAL(n.GetIntNumber(), 135, ());

    TEST(!n.IsIntersect(NumberT("134")), ());
    TEST(!n.IsIntersect(NumberT("136")), ());
  }


  {
    NumberT n("135-24", true);
    TEST(!n.IsOdd(), ());
    TEST_EQUAL(n.GetIntNumber(), 13524, ());
  }

  {
    NumberT n("135;133;131");
    TEST(n.IsOdd(), ());
    TEST_EQUAL(n.GetIntNumber(), 131, ());

    for (int i = 131; i <= 135; ++i)
      TEST(n.IsIntersect(NumberT(strings::to_string(i))), ());
    TEST(!n.IsIntersect(NumberT("130")), ());
    TEST(!n.IsIntersect(NumberT("136")), ());
  }

  {
    NumberT n("6-10", false);
    TEST(!n.IsOdd(), ());
    TEST_EQUAL(n.GetIntNumber(), 6, ());

    for (int i = 6; i <= 10; ++i)
      TEST(n.IsIntersect(NumberT(strings::to_string(i))), ());

    TEST(!n.IsIntersect(NumberT("5")), ());
    TEST(!n.IsIntersect(NumberT("11")), ());
  }
}

UNIT_TEST(HS_StreetsMerge)
{
  classificator::Load();

  FrozenDataSource dataSource;
  LocalCountryFile localFile(LocalCountryFile::MakeForTesting("minsk-pass"));
  // Clean indexes to avoid jenkins errors.
  platform::CountryIndexes::DeleteFromDisk(localFile);

  auto const p = dataSource.Register(localFile);
  TEST(p.first.IsAlive(), ());
  TEST_EQUAL(MwmSet::RegResult::Success, p.second, ());

  {
    search::HouseDetector houser(dataSource);
    StreetIDsByName toDo;
    toDo.streetNames.push_back("улица Володарского");
    dataSource.ForEachInScale([&toDo](FeatureType & ft) { toDo(ft); }, scales::GetUpperScale());
    houser.LoadStreets(toDo.GetFeatureIDs());
    TEST_EQUAL(houser.MergeStreets(), 1, ());
  }

  {
    search::HouseDetector houser(dataSource);
    StreetIDsByName toDo;
    toDo.streetNames.push_back("Московская улица");
    dataSource.ForEachInScale([&toDo](FeatureType & ft) { toDo(ft); }, scales::GetUpperScale());
    houser.LoadStreets(toDo.GetFeatureIDs());
    TEST_GREATER_OR_EQUAL(houser.MergeStreets(), 1, ());
    TEST_LESS_OR_EQUAL(houser.MergeStreets(), 3, ());
  }

  {
    search::HouseDetector houser(dataSource);
    StreetIDsByName toDo;
    toDo.streetNames.push_back("проспект Независимости");
    toDo.streetNames.push_back("Московская улица");
    dataSource.ForEachInScale([&toDo](FeatureType & ft) { toDo(ft); }, scales::GetUpperScale());
    houser.LoadStreets(toDo.GetFeatureIDs());
    TEST_GREATER_OR_EQUAL(houser.MergeStreets(), 1, ());
    TEST_LESS_OR_EQUAL(houser.MergeStreets(), 5, ());
  }

  {
    search::HouseDetector houser(dataSource);
    StreetIDsByName toDo;
    toDo.streetNames.push_back("проспект Независимости");
    toDo.streetNames.push_back("Московская улица");
    toDo.streetNames.push_back("Вишнёвый переулок");
    toDo.streetNames.push_back("Студенческий переулок");
    toDo.streetNames.push_back("Полоцкий переулок");
    dataSource.ForEachInScale([&toDo](FeatureType & ft) { toDo(ft); }, scales::GetUpperScale());
    houser.LoadStreets(toDo.GetFeatureIDs());
    TEST_GREATER_OR_EQUAL(houser.MergeStreets(), 1, ());
    TEST_LESS_OR_EQUAL(houser.MergeStreets(), 8, ());
  }

  {
    search::HouseDetector houser(dataSource);
    StreetIDsByName toDo;
    toDo.streetNames.push_back("проспект Независимости");
    toDo.streetNames.push_back("Московская улица");
    toDo.streetNames.push_back("улица Кирова");
    toDo.streetNames.push_back("улица Городской Вал");
    dataSource.ForEachInScale([&toDo](FeatureType & ft) { toDo(ft); }, scales::GetUpperScale());
    houser.LoadStreets(toDo.GetFeatureIDs());
    TEST_GREATER_OR_EQUAL(houser.MergeStreets(), 1, ());
    TEST_LESS_OR_EQUAL(houser.MergeStreets(), 10, ());
  }
}

namespace
{
m2::PointD FindHouse(DataSource & dataSource, vector<string> const & streets,
                     string const & houseName, double offset)
{
  search::HouseDetector houser(dataSource);

  StreetIDsByName toDo;
  toDo.streetNames = streets;
  dataSource.ForEachInScale([&toDo](FeatureType & ft) { toDo(ft); }, scales::GetUpperScale());

  if (houser.LoadStreets(toDo.GetFeatureIDs()) > 0)
    TEST_GREATER(houser.MergeStreets(), 0, ());

  houser.ReadAllHouses(offset);

  vector<search::HouseResult> houses;
  houser.GetHouseForName(houseName, houses);

  TEST_EQUAL(houses.size(), 1, (houses));
  return houses[0].m_house->GetPosition();
}

}

UNIT_TEST(HS_FindHouseSmoke)
{
  classificator::Load();

  FrozenDataSource dataSource;
  auto const p = dataSource.Register(LocalCountryFile::MakeForTesting("minsk-pass"));
  TEST(p.first.IsAlive(), ());
  TEST_EQUAL(MwmSet::RegResult::Success, p.second, ());

  {
    vector<string> streetName(1, "Московская улица");
    TEST_ALMOST_EQUAL_ULPS(FindHouse(dataSource, streetName, "7", 100),
                      m2::PointD(27.539850827603416406, 64.222406776416349317), ());
  }
  {
    vector<string> streetName(1, "проспект Независимости");
    TEST_ALMOST_EQUAL_ULPS(FindHouse(dataSource, streetName, "10", 40),
                      m2::PointD(27.551428582902474318, 64.234707387050306693), ());
  }
  {
    vector<string> streetName(1, "улица Ленина");

    /// @todo This cases doesn't work, but should in new search algorithms.
    //m2::PointD pt = FindHouse(dataSource, streetName, "28", 50);
    //m2::PointD pt = FindHouse(dataSource, streetName, "30", 50);

    m2::PointD pt = FindHouse(dataSource, streetName, "21", 50);
    TEST_ALMOST_EQUAL_ULPS(pt, m2::PointD(27.56477391395549148, 64.234502198059132638), ());
  }
}


UNIT_TEST(HS_StreetsCompare)
{
  search::Street A, B;
  TEST(search::Street::IsSameStreets(&A, &B), ());
  string str[8][2] = { {"Московская", "Московская"},
                       {"ул. Московская", "Московская ул."},
                       {"ул. Московская", "Московская улица"},
                       {"ул. Московская", "улица Московская"},
                       {"ул. Московская", "площадь Московская"},
                       {"ул. мОСКОВСКАЯ", "Московская улица"},
                       {"Московская", "площадь Московская"},
                       {"Московская         ", "аллея Московская"}
                     };
  for (size_t i = 0; i < ARRAY_SIZE(str); ++i)
  {
    A.SetName(str[i][0]);
    B.SetName(str[i][0]);
    TEST(search::Street::IsSameStreets(&A, &B), ());
  }
}

namespace
{
string GetStreetKey(string const & name)
{
  return strings::ToUtf8(search::GetStreetNameAsKey(name));
}
} // namespace

UNIT_TEST(HS_StreetKey)
{
  TEST_EQUAL("улицакрупскои", GetStreetKey("улица Крупской"), ());
  TEST_EQUAL("уручскаяул", GetStreetKey("Уручская ул."), ());
  TEST_EQUAL("пргазетыправда", GetStreetKey("Пр. Газеты Правда"), ());
  TEST_EQUAL("улицаякупалы", GetStreetKey("улица Я. Купалы"), ());
  TEST_EQUAL("францискаскоринытракт", GetStreetKey("Франциска Скорины Тракт"), ());
}

namespace
{

struct Address
{
  string m_streetKey;
  string m_house;
  double m_lat, m_lon;

  bool operator<(Address const & rhs) const { return (m_streetKey < rhs.m_streetKey); }
};

void swap(Address & a1, Address & a2)
{
  a1.m_streetKey.swap(a2.m_streetKey);
  a1.m_house.swap(a2.m_house);
  std::swap(a1.m_lat, a2.m_lat);
  std::swap(a1.m_lon, a2.m_lon);
}

}

UNIT_TEST(HS_MWMSearch)
{
  // "Minsk", "Belarus", "Lithuania", "USA_New York", "USA_California"
  string const country = "minsk-pass";

  string const path = GetPlatform().WritableDir() + country + ".addr";
  ifstream file(path.c_str());
  if (!file.good())
  {
    LOG(LWARNING, ("Address file not found", path));
    return;
  }

  FrozenDataSource dataSource;
  auto p = dataSource.Register(LocalCountryFile::MakeForTesting(country));
  if (p.second != MwmSet::RegResult::Success)
  {
    LOG(LWARNING, ("MWM file not found"));
    return;
  }
  TEST(p.first.IsAlive(), ());

  CollectStreetIDs streetIDs;
  dataSource.ForEachInScale(streetIDs, scales::GetUpperScale());
  streetIDs.Finish();

  string line;
  vector<Address> addresses;
  while (file.good())
  {
    getline(file, line);
    if (line.empty())
      continue;

    vector<string> v;
    strings::Tokenize(line, "|", MakeBackInsertFunctor(v));

    string key = GetStreetKey(v[0]);
    if (key.empty())
      continue;

    addresses.push_back(Address());
    Address & a = addresses.back();

    a.m_streetKey.swap(key);

    // House number is in v[1], sometime it contains house name after comma.
    strings::SimpleTokenizer house(v[1], ",");
    TEST(house, ());
    a.m_house = *house;
    TEST(!a.m_house.empty(), ());

    TEST(strings::to_double(v[2], a.m_lat), (v[2]));
    TEST(strings::to_double(v[3], a.m_lon), (v[3]));
  }

  sort(addresses.begin(), addresses.end());

  search::HouseDetector detector(dataSource);
  size_t all = 0, matched = 0, notMatched = 0;

  size_t const percent = max(size_t(1), addresses.size() / 100);
  for (size_t i = 0; i < addresses.size(); ++i)
  {
    if (i % percent == 0)
      LOG(LINFO, ("%", i / percent, "%"));

    Address const & a = addresses[i];

    vector<FeatureID> const & streets = streetIDs.Get(a.m_streetKey);
    if (streets.empty())
    {
      LOG(LWARNING, ("Missing street in mwm", a.m_streetKey));
      continue;
    }

    ++all;

    detector.LoadStreets(streets);
    detector.MergeStreets();
    detector.ReadAllHouses();

    vector<search::HouseResult> houses;
    detector.GetHouseForName(a.m_house, houses);
    if (houses.empty())
    {
      LOG(LINFO, ("No houses", a.m_streetKey, a.m_house));
      continue;
    }

    size_t j = 0;
    size_t const count = houses.size();
    for (; j < count; ++j)
    {
      search::House const * h = houses[j].m_house;
      m2::PointD p = h->GetPosition();
      p.x = MercatorBounds::XToLon(p.x);
      p.y = MercatorBounds::YToLat(p.y);

      //double const eps = 3.0E-4;
      //if (fabs(p.x - a.m_lon) < eps && fabs(p.y - a.m_lat) < eps)
      if (ms::DistanceOnEarth(a.m_lat, a.m_lon, p.y, p.x) < 3.0)
      {
        ++matched;
        break;
      }
    }

    if (j == count)
    {
      ++notMatched;
      LOG(LINFO, ("Bad matched", a.m_streetKey, a.m_house));
    }
  }

  LOG(LINFO, ("Matched =", matched, "Not matched =", notMatched, "Not found =", all - matched - notMatched));
  LOG(LINFO, ("All count =", all, "Percent matched =", matched / double(all)));
}

#include "../../testing/testing.hpp"

#include "../house_detector.hpp"
#include "../ftypes_matcher.hpp"

#include "../../base/logging.hpp"

#include "../../platform/platform.hpp"

#include "../../geometry/distance_on_sphere.hpp"

#include "../../indexer/scales.hpp"
#include "../../indexer/index.hpp"
#include "../../indexer/classificator_loader.hpp"

#include "../../std/iostream.hpp"
#include "../../std/fstream.hpp"


class StreetIDsByName
{
  vector<FeatureID> vect;

public:
  vector<string> streetNames;

  void operator() (FeatureType const & f)
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
    search::GetStreetNameAsKey(name, key);

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
  void operator() (FeatureType const & f)
  {
    static ftypes::IsStreetChecker checker;

    if (f.GetFeatureType() == feature::GEOM_LINE)
    {
      string name;
      if (f.GetName(0, name) && checker(f))
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

UNIT_TEST(HS_StreetsMerge)
{
  classificator::Load();

  Index index;
  m2::RectD rect;

  TEST(index.Add("minsk-pass.mwm", rect), ());

  {
    search::HouseDetector houser(&index);
    StreetIDsByName toDo;
    toDo.streetNames.push_back("улица Володарского");
    index.ForEachInScale(toDo, scales::GetUpperScale());
    houser.LoadStreets(toDo.GetFeatureIDs());
    TEST_EQUAL(houser.MergeStreets(), 1, ());
  }

  {
    search::HouseDetector houser(&index);
    StreetIDsByName toDo;
    toDo.streetNames.push_back("Московская улица");
    index.ForEachInScale(toDo, scales::GetUpperScale());
    houser.LoadStreets(toDo.GetFeatureIDs());
    TEST_LESS_OR_EQUAL(houser.MergeStreets(), 3, ());
  }

  {
    search::HouseDetector houser(&index);
    StreetIDsByName toDo;
    toDo.streetNames.push_back("проспект Независимости");
    toDo.streetNames.push_back("Московская улица");
    index.ForEachInScale(toDo, scales::GetUpperScale());
    houser.LoadStreets(toDo.GetFeatureIDs());
    TEST_LESS_OR_EQUAL(houser.MergeStreets(), 5, ());
  }

  {
    search::HouseDetector houser(&index);
    StreetIDsByName toDo;
    toDo.streetNames.push_back("проспект Независимости");
    toDo.streetNames.push_back("Московская улица");
    toDo.streetNames.push_back("Вишнёвый переулок");
    toDo.streetNames.push_back("Студенческий переулок");
    toDo.streetNames.push_back("Полоцкий переулок");
    index.ForEachInScale(toDo, scales::GetUpperScale());
    houser.LoadStreets(toDo.GetFeatureIDs());
    TEST_LESS_OR_EQUAL(houser.MergeStreets(), 8, ());
  }

  {
    search::HouseDetector houser(&index);
    StreetIDsByName toDo;
    toDo.streetNames.push_back("проспект Независимости");
    toDo.streetNames.push_back("Московская улица");
    toDo.streetNames.push_back("улица Кирова");
    toDo.streetNames.push_back("улица Городской Вал");
    index.ForEachInScale(toDo, scales::GetUpperScale());
    houser.LoadStreets(toDo.GetFeatureIDs());
    TEST_LESS_OR_EQUAL(houser.MergeStreets(), 10, ());
  }
}

namespace
{

m2::PointD FindHouse(Index & index, vector<string> const & streets,
                     string const & houseName, double offset)
{
  search::HouseDetector houser(&index);

  StreetIDsByName toDo;
  toDo.streetNames = streets;
  index.ForEachInScale(toDo, scales::GetUpperScale());

  if (houser.LoadStreets(toDo.GetFeatureIDs()) > 0)
    TEST_GREATER(houser.MergeStreets(), 0, ());

  houser.ReadAllHouses(offset);

  vector<search::House const *> houses;
  houser.GetHouseForName(houseName, houses);

  TEST_EQUAL(houses.size(), 1, ());
  return houses[0]->GetPosition();
}

}

UNIT_TEST(HS_FindHouseSmoke)
{
  classificator::Load();

  Index index;
  m2::RectD rect;
  index.Add("minsk-pass.mwm", rect);

  {
    vector<string> streetName(1, "Московская улица");
    TEST_ALMOST_EQUAL(FindHouse(index, streetName, "7", 100),
                      m2::PointD(27.539850827603416406, 64.222406776416349317), ());
  }
  {
    vector<string> streetName(1, "проспект Независимости");
    TEST_ALMOST_EQUAL(FindHouse(index, streetName, "10", 40),
                      m2::PointD(27.551358845467561309, 64.234708728154814139), ());
  }
  {
    vector<string> streetName(1, "улица Ленина");
    TEST_ALMOST_EQUAL(FindHouse(index, streetName, "9", 50),
                      m2::PointD(27.560341563525355468, 64.240918042070561), ());
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
  string res;
  search::GetStreetNameAsKey(name, res);
  return res;
}

}

UNIT_TEST(HS_StreetKey)
{
  TEST_EQUAL("крупской", GetStreetKey("улица Крупской"), ());
  TEST_EQUAL("уручская", GetStreetKey("Уручская ул."), ());
  TEST_EQUAL("газетыправда", GetStreetKey("Пр. Газеты Правда"), ());
  TEST_EQUAL("якупалы", GetStreetKey("улица Я. Купалы"), ());
  TEST_EQUAL("францискаскорины", GetStreetKey("Франциска Скорины Тракт"), ());
}

namespace
{

struct Address
{
  string m_streetKey;
  string m_house;
  double m_lat, m_lon;

  bool operator<(Address const & rhs) const
  {
    return (m_streetKey < rhs.m_streetKey);
  }
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
  string const path = GetPlatform().WritableDir() + "addresses-Minsk.txt";
  ifstream file(path.c_str());
  if (!file.good())
  {
    LOG(LWARNING, ("Address file not found", path));
    return;
  }

  Index index;
  m2::RectD rect;
  if (!index.Add("Minsk.mwm", rect))
  {
    LOG(LWARNING, ("MWM file not found"));
    return;
  }

  CollectStreetIDs streetIDs;
  index.ForEachInScale(streetIDs, scales::GetUpperScale());
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

  search::HouseDetector detector(&index);
  size_t all = 0, matched = 0, notMatched = 0;

  size_t const percent = addresses.size() / 100;
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
    detector.ReadAllHouses(200);

    vector<search::House const *> houses;
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
      m2::PointD p = houses[j]->GetPosition();
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

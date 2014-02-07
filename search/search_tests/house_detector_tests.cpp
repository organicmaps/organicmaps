#include "../../testing/testing.hpp"

#include "../house_detector.hpp"
#include "../ftypes_matcher.hpp"

#include "../../base/logging.hpp"

#include "../../platform/platform.hpp"

#include "../../indexer/scales.hpp"
#include "../../indexer/index.hpp"
#include "../../indexer/classificator_loader.hpp"

#include "../../std/iostream.hpp"
#include "../../std/fstream.hpp"


UNIT_TEST(HS_LessPoints)
{
  double q = 3.0 * 360.0 / 40.0E06;
  search::HouseDetector::LessWithEpsilon compare(&q);
  {
    m2::PointD a(1, 1);
    m2::PointD b(2, 2);
    TEST(compare(a, b), ());
    TEST(!compare(b, a), ());
  }
  {
    m2::PointD a(1, 1);
    m2::PointD b(1, 1);
    TEST(!compare(a, b), ());
    TEST(!compare(b, a), ());
  }
  {
    m2::PointD a(1, 1);
    m2::PointD b(1.1, 1.1);
    TEST(compare(a, b), ());
    TEST(!compare(b, a), ());
  }
  {
    m2::PointD a(1, 1);
    m2::PointD b(1 + q, 1);
    TEST(!compare(a, b), ());
    TEST(!compare(b, a), ());
  }
  {
    m2::PointD a(1, 1);
    m2::PointD b(1 + q, 1 - q);
    TEST(!compare(a, b), ());
    TEST(!compare(b, a), ());
  }
  {
    m2::PointD a(1, 1 - q);
    m2::PointD b(1 + q, 1);
    TEST(!compare(a, b), ());
    TEST(!compare(b, a), ());
  }
  {
    m2::PointD a(1 - q, 1 - q);
    m2::PointD b(1, 1);
    TEST(!compare(a, b), ());
    TEST(!compare(b, a), ());
  }
  {
    m2::PointD a(1, 1);
    m2::PointD b(1 + q, 1 + q);
    TEST(!compare(a, b), ());
    TEST(!compare(b, a), ());
  }
  {
    m2::PointD a(1, 1);
    m2::PointD b(1 + 2 * q, 1 + q);
    TEST(compare(a, b), ());
    TEST(!compare(b, a), ());
  }
}

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

  vector<FeatureID> const & Get(string const & name) const
  {
    string key;
    if (!GetKey(name, key))
      return m_empty;

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

bool LessHouseNumber(search::House const & h1, search::House const & h2)
{
  return search::House::LessHouseNumber()(&h1, &h2);
}

string GetStreetKey(string const & name)
{
  string res;
  search::GetStreetNameAsKey(name, res);
  return res;
}

}

UNIT_TEST(HS_HousesCompare)
{
  m2::PointD p(1,1);
  TEST(LessHouseNumber(search::House("1", p), search::House("2", p)), ());
  TEST(LessHouseNumber(search::House("18a", p), search::House("18b", p)), ());
  TEST(LessHouseNumber(search::House("120 1A", p), search::House("120 7A", p)), ());
  TEST(LessHouseNumber(search::House("120 1A", p), search::House("120 7B", p)), ());

  TEST(!LessHouseNumber(search::House("4", p), search::House("4", p)), ());
  TEST(!LessHouseNumber(search::House("95", p), search::House("82-b", p)), ());

  TEST(!LessHouseNumber(search::House("2", p), search::House("1", p)), ());
  TEST(!LessHouseNumber(search::House("18b", p), search::House("18a", p)), ());
  TEST(!LessHouseNumber(search::House("120 7A", p), search::House("120 1A", p)), ());
  TEST(!LessHouseNumber(search::House("120 7B", p), search::House("120 1A", p)), ());
}

UNIT_TEST(HS_StreetKey)
{
  TEST_EQUAL("крупской", GetStreetKey("улица Крупской"), ());
  TEST_EQUAL("уручская", GetStreetKey("Уручская ул."), ());
  TEST_EQUAL("газетыправда", GetStreetKey("Пр. Газеты Правда"), ());
  TEST_EQUAL("якупалы", GetStreetKey("улица Я. Купалы"), ());
  TEST_EQUAL("францискаскорины", GetStreetKey("Франциска Скорины Тракт"), ());
}

UNIT_TEST(HS_MWMSearch)
{
  string const path = GetPlatform().WritableDir() + "minsk-pass.addr";
  ifstream file(path.c_str());
  if (!file.good())
  {
    LOG(LWARNING, ("Address file not found"));
    return;
  }

  Index index;
  m2::RectD rect;
  if (!index.Add("minsk-pass.mwm", rect))
  {
    LOG(LWARNING, ("MWM file not found"));
    return;
  }

  CollectStreetIDs streetIDs;
  index.ForEachInScale(streetIDs, scales::GetUpperScale());
  streetIDs.Finish();

  search::HouseDetector detector(&index);

  size_t all = 0, matched = 0, notMatched = 0;
  set<string> addrSet;

  string line;
  while (file.good())
  {
    getline(file, line);
    if (line.empty())
      continue;

    vector<string> v;
    strings::Tokenize(line, "|", MakeBackInsertFunctor(v));

    // House number is in v[1], sometime it contains house name after comma.
    strings::SimpleTokenizer house(v[1], ",");
    TEST(house, ());
    v[1] = *house;

    TEST(!v[0].empty(), ());
    TEST(!v[1].empty(), ());

    if (!addrSet.insert(v[0] + v[1]).second)
      continue;

    vector<FeatureID> const & streets = streetIDs.Get(v[0]);
    if (streets.empty())
    {
      LOG(LWARNING, ("Missing street in mwm", v[0]));
      continue;
    }

    ++all;

    detector.LoadStreets(streets);
    detector.MergeStreets();
    detector.ReadAllHouses(200);

    vector<search::House const *> houses;
    detector.GetHouseForName(v[1], houses);
    if (houses.empty())
    {
      LOG(LINFO, ("No houses", v[0], v[1]));
      continue;
    }

    double lat, lon;
    TEST(strings::to_double(v[2], lat), (v[2]));
    TEST(strings::to_double(v[3], lon), (v[3]));

    size_t i = 0;
    size_t const count = houses.size();
    for (; i < count; ++i)
    {
      m2::PointD p = houses[i]->GetPosition();
      p.x = MercatorBounds::XToLon(p.x);
      p.y = MercatorBounds::YToLat(p.y);

      double const eps = 1.0E-4;
      if (fabs(p.x - lon) < eps && fabs(p.y - lat) < eps)
      {
        ++matched;
        break;
      }
    }

    if (i == count)
    {
      ++notMatched;
      LOG(LINFO, ("Bad matched", v[0], v[1]));
    }
  }

  LOG(LINFO, ("Matched =", matched, "Not matched =", notMatched, "Not found =", all - matched - notMatched));
  LOG(LINFO, ("All count =", all, "Percent matched =", matched / double(all)));
}

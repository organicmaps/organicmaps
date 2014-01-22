#include "../../testing/testing.hpp"

#include "../../base/logging.hpp"

#include "../../platform/platform.hpp"

#include "../../indexer/scales.hpp"
#include "../../indexer/index.hpp"
#include "../../indexer/classificator_loader.hpp"

#include "../house_detector.hpp"

#include "../../std/iostream.hpp"

UNIT_TEST(LESS_WITH_EPSILON)
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

class Process
{
  vector<FeatureID> vect;

public:
  vector<string> streetNames;

  void operator() (FeatureType const & f)
  {
    if (f.GetFeatureType() == feature::GEOM_LINE)
    {
      string name;
      if (f.GetName(0, name))
       {
         for (size_t i = 0; i < streetNames.size(); ++i)
           if (name == streetNames[i])
           {
             vect.push_back(f.GetID());
             break;
           }
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

UNIT_TEST(STREET_MERGE_TEST)
{
  classificator::Load();

  Index index;
  m2::RectD rect;

  TEST(index.Add("minsk-pass.mwm", rect), ());

  {
    search::HouseDetector houser(&index);
    Process toDo;
    toDo.streetNames.push_back("улица Володарского");
    index.ForEachInScale(toDo, scales::GetUpperScale());
    houser.LoadStreets(toDo.GetFeatureIDs());
    TEST_EQUAL(houser.MergeStreets(), 1, ());
  }

  {
    search::HouseDetector houser(&index);
    Process toDo;
    toDo.streetNames.push_back("Московская улица");
    index.ForEachInScale(toDo, scales::GetUpperScale());
    houser.LoadStreets(toDo.GetFeatureIDs());
    TEST_LESS_OR_EQUAL(houser.MergeStreets(), 3, ());
  }

  {
    search::HouseDetector houser(&index);
    Process toDo;
    toDo.streetNames.push_back("проспект Независимости");
    toDo.streetNames.push_back("Московская улица");
    index.ForEachInScale(toDo, scales::GetUpperScale());
    houser.LoadStreets(toDo.GetFeatureIDs());
    TEST_LESS_OR_EQUAL(houser.MergeStreets(), 5, ());
  }

  {
    search::HouseDetector houser(&index);
    Process toDo;
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
    Process toDo;
    toDo.streetNames.push_back("проспект Независимости");
    toDo.streetNames.push_back("Московская улица");
    toDo.streetNames.push_back("улица Кирова");
    toDo.streetNames.push_back("улица Городской Вал");
    index.ForEachInScale(toDo, scales::GetUpperScale());
    houser.LoadStreets(toDo.GetFeatureIDs());
    TEST_LESS_OR_EQUAL(houser.MergeStreets(), 9, ());
  }
}

namespace
{

m2::PointD FindHouse(Index & index, vector<string> const & streets,
                     string const & houseName, double offset)
{
  search::HouseDetector houser(&index);

  Process toDo;
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

UNIT_TEST(SEARCH_HOUSE_NUMBER_SMOKE_TEST)
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


UNIT_TEST(STREET_COMPARE_TEST)
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

}

UNIT_TEST(HOUSE_COMPARE_TEST)
{
  m2::PointD p(1,1);
  TEST(LessHouseNumber(search::House("1", p), search::House("2", p)), ());
//  TEST(LessHouseNumber(search::House("18a", p), search::House("18b", p)), ());
//  TEST(LessHouseNumber(search::House("120 1A", p), search::House("120 7A", p)), ());
//  TEST(LessHouseNumber(search::House("120 1A", p), search::House("120 7B", p)), ());

  TEST(!LessHouseNumber(search::House("4", p), search::House("4", p)), ());
  TEST(!LessHouseNumber(search::House("95", p), search::House("82-b", p)), ());

  TEST(!LessHouseNumber(search::House("2", p), search::House("1", p)), ());
  TEST(!LessHouseNumber(search::House("18b", p), search::House("18a", p)), ());
  TEST(!LessHouseNumber(search::House("120 7A", p), search::House("120 1A", p)), ());
  TEST(!LessHouseNumber(search::House("120 7B", p), search::House("120 1A", p)), ());
}

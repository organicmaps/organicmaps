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

struct Process
{
  vector <FeatureID> vect;
  vector <string> streetNames;
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
};

UNIT_TEST(STREET_MERGE_TEST)
{
  classificator::Load();

  Index index;
  m2::RectD rect;

  index.Add("minsk-pass.mwm", rect);
  {
    search::HouseDetector houser(&index);
    Process toDo;
    toDo.streetNames.push_back("Московская улица");
    index.ForEachInScale(toDo, scales::GetUpperScale());
    houser.LoadStreets(toDo.vect);
    TEST_EQUAL(houser.MergeStreets(), 1, ());
  }

  {
    search::HouseDetector houser(&index);
    Process toDo;
    toDo.streetNames.push_back("проспект Независимости");
    toDo.streetNames.push_back("Московская улица");
    index.ForEachInScale(toDo, scales::GetUpperScale());
    houser.LoadStreets(toDo.vect);
    TEST_EQUAL(houser.MergeStreets(), 2, ());
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
    houser.LoadStreets(toDo.vect);
    TEST_EQUAL(houser.MergeStreets(), 5, ());
  }
  {
    search::HouseDetector houser(&index);
    Process toDo;
    toDo.streetNames.push_back("проспект Независимости");
    toDo.streetNames.push_back("Московская улица");
    toDo.streetNames.push_back("улица Кирова");
    toDo.streetNames.push_back("улица Городской Вал");
    index.ForEachInScale(toDo, scales::GetUpperScale());
    houser.LoadStreets(toDo.vect);
    TEST_EQUAL(houser.MergeStreets(), 4, ());
  }
}

namespace
{

void GetCanditates(Index & index, vector<string> const & v, string const & houseName,
                   vector<search::HouseProjection> & res, double offset)
{
  search::HouseDetector houser(&index);
  Process toDo;
  toDo.streetNames = v;
  index.ForEachInScale(toDo, scales::GetUpperScale());

  houser.LoadStreets(toDo.vect);
  houser.ReadAllHouses(offset);
  houser.MatchAllHouses(houseName, res);

  sort(res.begin(), res.end(), &search::HouseProjection::LessDistance);
}

}

UNIT_TEST(SEARCH_HOUSE_NUMBER_SMOKE_TEST)
{
  classificator::Load();

  Index index;
  m2::RectD rect;
  index.Add("minsk-pass.mwm", rect);

  {
    vector <string> streetName(1, "Московская улица");
    vector <search::HouseProjection> res;
    string houseName = "7";
    GetCanditates(index, streetName, houseName, res, 100);
    TEST_EQUAL(res.size(), 1, ());
    TEST_ALMOST_EQUAL(res[0].m_house->GetPosition(), m2::PointD(27.539850827603416406, 64.222406776416349317), ());
    TEST_EQUAL(res[0].m_house->GetNumber(), houseName, ());
  }
  {
    vector <string> streetName(1, "проспект Независимости");
    vector <search::HouseProjection> res;
    string houseName = "10";
    GetCanditates(index, streetName, houseName, res, 40);
    TEST_EQUAL(res.size(), 1, ());
    TEST_ALMOST_EQUAL(res[0].m_house->GetPosition(), m2::PointD(27.551358845467561309, 64.234708728154814139), ());
    TEST_EQUAL(res[0].m_house->GetNumber(), houseName, ());
  }
  {
    vector <string> streetName(1, "улица Ленина");
    vector <search::HouseProjection> res;
    string houseName = "9";
    GetCanditates(index, streetName, houseName, res, 50);
    TEST_EQUAL(res.size(), 1, ());
    TEST_ALMOST_EQUAL(res[0].m_house->GetPosition(), m2::PointD(27.560341563525355468, 64.240918042070561), ());
    TEST_EQUAL(res[0].m_house->GetNumber(), houseName, ());
  }
}


UNIT_TEST(STREET_COMPARE_TEST)
{
  search::Street A, B;
  TEST(search::Street::IsSameStreets(A, B), ());
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
    TEST(search::Street::IsSameStreets(A, B), ());
  }
}

UNIT_TEST(HOUSE_COMPARE_TEST)
{
  m2::PointD p(1,1);
  TEST(search::House::LessHouseNumber(search::House("1", p), search::House("2", p)), ());
  TEST(search::House::LessHouseNumber(search::House("123", p), search::House("123-3", p)), ());
  TEST(search::House::LessHouseNumber(search::House("18a", p), search::House("18b", p)), ());
  TEST(search::House::LessHouseNumber(search::House("120 1A", p), search::House("120 7A", p)), ());
  TEST(search::House::LessHouseNumber(search::House("120 1A", p), search::House("120 7B", p)), ());

  TEST(!search::House::LessHouseNumber(search::House("4", p), search::House("4", p)), ());
  TEST(!search::House::LessHouseNumber(search::House("95", p), search::House("82-b", p)), ());

  TEST(!search::House::LessHouseNumber(search::House("2", p), search::House("1", p)), ());
  TEST(!search::House::LessHouseNumber(search::House("123-3", p), search::House("123", p)), ());
  TEST(!search::House::LessHouseNumber(search::House("18b", p), search::House("18a", p)), ());
  TEST(!search::House::LessHouseNumber(search::House("120 7A", p), search::House("120 1A", p)), ());
  TEST(!search::House::LessHouseNumber(search::House("120 7B", p), search::House("120 1A", p)), ());
}

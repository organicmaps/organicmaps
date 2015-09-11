#include "testing/testing.hpp"

#include "indexer/drules_city_rank_table.hpp"

UNIT_TEST(TestCityRankTableParser)
{
  string s =
    "z-4{\n"
    "-10000:0.25;\n"
    "10000-1000000:0.5;\n}\n"
    " z5 { 1000-10000 : -1; } "
    "z6-15{\n"
    "-10000:0.25;\n"
    "10000-:0.5;\n}\n"
    "z16-{\n"
    "-10000:0.25;\n"
    "10000-10000000:0.5;\n}\n";

  unique_ptr<drule::ICityRankTable> table = drule::GetCityRankTableFromString(s);
  TEST(nullptr != table.get(), ());

  double rank = 0.0;

  // test zoom > 19

  // test there is no rank for zoom > 19
  TEST_EQUAL(false, table->GetCityRank(20, 100000, rank), ());

  // test zoom 0-4

  // test there is 0.25 rank for city with population = 5K and zoom = 3
  TEST_EQUAL(true, table->GetCityRank(3, 5000, rank), ());
  TEST_EQUAL(0.25, rank, ());

  // test there is 0.5 rank for city with population = 50K and zoom = 3
  TEST_EQUAL(true, table->GetCityRank(3, 50000, rank), ());
  TEST_EQUAL(0.5, rank, ());

  // test there is no rank for city with population = 5M and zoom = 3
  TEST_EQUAL(false, table->GetCityRank(3, 50000000, rank), ());

  // test zoom 5

  // test there is no rank for city with population = 500 and zoom = 5
  TEST_EQUAL(false, table->GetCityRank(5, 500, rank), ());

  // test there is -1 rank for city with population = 5K and zoom = 5
  TEST_EQUAL(true, table->GetCityRank(5, 5000, rank), ());
  TEST_EQUAL(-1.0, rank, ());

  // test there is no rank for city with population = 50K and zoom = 5
  TEST_EQUAL(false, table->GetCityRank(5, 50000, rank), ());

  // test zoom 6-15

  // test there is 0.25 rank for city with population = 5K and zoom = 9
  TEST_EQUAL(true, table->GetCityRank(9, 5000, rank), ());
  TEST_EQUAL(0.25, rank, ());

  // test there is 0.5 rank for city with population = 50K and zoom = 9
  TEST_EQUAL(true, table->GetCityRank(9, 50000, rank), ());
  TEST_EQUAL(0.5, rank, ());

  // test zoom 16-19

  // test there is 0.25 rank for city with population = 5K and zoom = 17
  TEST_EQUAL(true, table->GetCityRank(17, 5000, rank), ());
  TEST_EQUAL(0.25, rank, ());

  // test there is 0.5 rank for city with population = 50K and zoom = 17
  TEST_EQUAL(true, table->GetCityRank(17, 50000, rank), ());
  TEST_EQUAL(0.5, rank, ());

  // test there is no rank for city with population = 50M and zoom = 17
  TEST_EQUAL(false, table->GetCityRank(17, 50000000, rank), ());
}

UNIT_TEST(TestCityRankTableParserInvalidString1)
{
  string s = "z-5{bad_format;}";

  unique_ptr<drule::ICityRankTable> table = drule::GetCityRankTableFromString(s);
  TEST(nullptr == table.get(), ());
}

UNIT_TEST(TestCityRankTableParserInvalidString2)
{
  string s = "z-5{0-1000:0.25;} zBadFormat{}";

  unique_ptr<drule::ICityRankTable> table = drule::GetCityRankTableFromString(s);
  TEST(nullptr == table.get(), ());
}

UNIT_TEST(TestCityRankTableWithEmptyPopulation1)
{
  string s = "z-5  {} z6-9 {;;;1000-10000:0.25;;;}\n"
             "z10-{ -:3.5;;; }";

  unique_ptr<drule::ICityRankTable> table = drule::GetCityRankTableFromString(s);
  TEST(nullptr != table.get(), ());

  double rank = 0.0;

  // there is no rank for zoom 0-5
  TEST_EQUAL(false, table->GetCityRank(3, 100000, rank), ());

  // there is no rank for zoom 6-9 and population 100
  TEST_EQUAL(false, table->GetCityRank(7, 100, rank), ());

  // there is 0.25 rank for zoom 6-9 and population 5000
  TEST_EQUAL(true, table->GetCityRank(7, 5000, rank), ());
  TEST_EQUAL(0.25, rank, ());

  // there is no rank for zoom 6-9 and population 15000
  TEST_EQUAL(false, table->GetCityRank(7, 15000, rank), ());

  // there is 3.5 rank for zoom 10+ and any population
  TEST_EQUAL(true, table->GetCityRank(17, 1, rank), ());
  TEST_EQUAL(3.5, rank, ());
}

UNIT_TEST(TestCityRankTableWithEmptyPopulation2)
{
  string s = "z0-20  {}";

  unique_ptr<drule::ICityRankTable> table = drule::GetCityRankTableFromString(s);
  TEST(nullptr != table.get(), ());

  // there is no any rank
  double rank = 0.0;
  TEST_EQUAL(false, table->GetCityRank(3, 100000, rank), ());
}

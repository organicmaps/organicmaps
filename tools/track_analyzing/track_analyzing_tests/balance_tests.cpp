#include "testing/testing.hpp"

#include "track_analyzing/track_analyzer/cmd_balance_csv.hpp"

#include <algorithm>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

namespace
{
using namespace std;
using namespace track_analyzing;

string const csv =
    R"(user,mwm,hw type,surface type,maxspeed km/h,is city road,is one way,is day,lat lon,distance,time,mean speed km/h,turn from smaller to bigger,turn from bigger to smaller,from link,to link,intersection with big,intersection with small,intersection with link
I:D923B4DC-09E0-4B0B-B7F8-153965396B55,New Zealand,highway-trunk,psurface-paved_good,80,0,0,1,-41.4832 173.844,163.984,6,98.3902,0,0,0,0,0,1,0
I:D923B4DC-09E0-4B0B-B7F8-153965396B55,New Zealand,highway-trunk,psurface-paved_good,80,0,0,1,-41.4831 173.845,2274.59,108,75.8196,0,0,0,0,0,4,0
A:b6e31294-5c90-4105-9f7b-51a382eabfa0,Poland,highway-primary,psurface-paved_good,50,0,0,0,53.8524 21.3061,1199.34,60,71.9604,0,0,0,0,0,10,0)";

UNIT_TEST(FillTableTestEmpty)
{
  std::stringstream ss;
  std::vector<TableRow> table;
  MwmToDataPoints matchedDataPoints;

  // Stream without csv header.
  FillTable(ss, matchedDataPoints, table);
  TEST(table.empty(), ());
  TEST(matchedDataPoints.empty(), ());

  // Stream with only csv header.
  ss << "mwm,number\n";
  FillTable(ss, matchedDataPoints, table);
  TEST(table.empty(), ());
  TEST(matchedDataPoints.empty(), ());
}

UNIT_TEST(FillTableTest)
{
  std::stringstream ss;
  std::vector<TableRow> table;
  MwmToDataPoints matchedDataPoints;

  ss << csv;
  FillTable(ss, matchedDataPoints, table);

  MwmToDataPoints const expectedMatchedDataPoints = {{"New Zealand", 2 /* data points */},
                                                     {"Poland", 1 /* data points */}};
  std::vector<TableRow> expectedTable = {
      {"I:D923B4DC-09E0-4B0B-B7F8-153965396B55", "New Zealand", "highway-trunk", "psurface-paved_good", "80", "0", "0",
       "1", "-41.4832 173.844", "163.984", "6", "98.3902", "0", "0", "0", "0", "0", "1", "0"},
      {"I:D923B4DC-09E0-4B0B-B7F8-153965396B55", "New Zealand", "highway-trunk", "psurface-paved_good", "80", "0", "0",
       "1", "-41.4831 173.845", "2274.59", "108", "75.8196", "0", "0", "0", "0", "0", "4", "0"},
      {"A:b6e31294-5c90-4105-9f7b-51a382eabfa0", "Poland", "highway-primary", "psurface-paved_good", "50", "0", "0",
       "0", "53.8524 21.3061", "1199.34", "60", "71.9604", "0", "0", "0", "0", "0", "10", "0"}};

  TEST_EQUAL(matchedDataPoints, expectedMatchedDataPoints, ());
  TEST_EQUAL(table, expectedTable, ());
}

UNIT_TEST(AreKeysEqualEmptyMapsTest)
{
  TEST(AreKeysEqual(MwmToDataPoints(), MwmToDataPoints()), ());
}

UNIT_TEST(AreKeysEqualTest)
{
  MwmToDataPoints const map1 = {{"Russia_Moscow", 5 /* data points */}, {"San Marino", 7 /* data points */}};
  MwmToDataPoints map2 = {{"Russia_Moscow", 7 /* data points*/}, {"San Marino", 9 /* data points */}};
  TEST(AreKeysEqual(map1, map2), ());

  map2["Slovakia"] = 3;
  TEST(!AreKeysEqual(map1, map2), ());
}

UNIT_TEST(RemoveKeysSmallValueEmptyTest)
{
  MwmToDataPoints checkedMap;
  MwmToDataPoints additionalMap;
  RemoveKeysSmallValue(checkedMap, additionalMap, 10 /* ignoreDataPointNumber */);
  TEST(checkedMap.empty(), (checkedMap.size()));
  TEST(additionalMap.empty(), (checkedMap.size()));
}

UNIT_TEST(RemoveKeysSmallValueTest)
{
  MwmToDataPoints checkedMap = {{"Russia_Moscow", 3 /* data points */}, {"San Marino", 5 /* data points */}};
  MwmToDataPoints additionalMap = {{"Russia_Moscow", 7 /* data points*/}, {"San Marino", 9 /* data points */}};
  RemoveKeysSmallValue(checkedMap, additionalMap, 4 /* ignoreDataPointNumber */);

  MwmToDataPoints expectedCheckedMap = {{"San Marino", 5 /* data points */}};
  MwmToDataPoints expectedAdditionalMap = {{"San Marino", 9 /* data points */}};

  TEST_EQUAL(checkedMap, expectedCheckedMap, ());
  TEST_EQUAL(additionalMap, expectedAdditionalMap, ());
}

UNIT_TEST(GetMwmToDataPointFractionTest)
{
  MwmToDataPoints const numberMapping = {{"Russia_Moscow", 100 /* data points */},
                                         {"San Marino", 50 /* data points */},
                                         {"Slovakia", 50 /* data points */}};
  auto const fractionMapping = GetMwmToDataPointFraction(numberMapping);

  MwmToDataPointFraction expectedFractionMapping{
      {"Russia_Moscow", 0.5 /* fraction */}, {"San Marino", 0.25 /* fraction */}, {"Slovakia", 0.25 /* fraction */}};
  TEST_EQUAL(fractionMapping, expectedFractionMapping, ());
}

UNIT_TEST(CalcsMatchedDataPointsToKeepDistributionTest)
{
  {
    MwmToDataPoints const matchedDataPoints = {{"Russia_Moscow", 400 /* data points */},
                                               {"San Marino", 400 /* data points */},
                                               {"Slovakia", 200 /* data points */}};
    MwmToDataPointFraction const distributionFraction = {{"Russia_Moscow", 0.8 /* data points */},
                                                         {"San Marino", 0.1 /* data points */},
                                                         {"Slovakia", 0.1 /* data points */}};
    MwmToDataPoints expected = {{"Russia_Moscow", 400 /* data points */},
                                {"San Marino", 50 /* data points */},
                                {"Slovakia", 50 /* data points */}};
    MwmToDataPoints const matchedDataPointsToKeepDistribution =
        CalcsMatchedDataPointsToKeepDistribution(matchedDataPoints, distributionFraction);
    TEST_EQUAL(matchedDataPointsToKeepDistribution, expected, ());
  }
  {
    MwmToDataPoints const matchedDataPoints = {{"Russia_Moscow", 800 /* data points */},
                                               {"San Marino", 100 /* data points */},
                                               {"Slovakia", 100 /* data points */}};
    MwmToDataPointFraction const distributionFraction = {{"Russia_Moscow", 0.2 /* data points */},
                                                         {"San Marino", 0.4 /* data points */},
                                                         {"Slovakia", 0.4 /* data points */}};
    MwmToDataPoints expected = {{"Russia_Moscow", 50 /* data points */},
                                {"San Marino", 100 /* data points */},
                                {"Slovakia", 100 /* data points */}};
    MwmToDataPoints const matchedDataPointsToKeepDistribution =
        CalcsMatchedDataPointsToKeepDistribution(matchedDataPoints, distributionFraction);
    TEST_EQUAL(matchedDataPointsToKeepDistribution, expected, ());
  }
}

UNIT_TEST(BalancedDataPointNumberTheSameTest)
{
  MwmToDataPoints const distribution = {{"Russia_Moscow", 100 /* data points */},
                                        {"San Marino", 50 /* data points */},
                                        {"Slovakia", 50 /* data points */}};
  MwmToDataPoints const matchedDataPoints = {
      {"Russia_Moscow", 10 /* data points */}, {"San Marino", 5 /* data points */}, {"Slovakia", 5 /* data points */}};
  {
    // Case when the distribution is not changed. All mwms lose the same percent of data points.
    auto distr = distribution;
    auto matched = matchedDataPoints;
    auto const balancedDataPointNumber =
        BalancedDataPointNumber(std::move(matched), std::move(distr), 0 /* ignoreDataPointsNumber */);
    TEST_EQUAL(balancedDataPointNumber, matchedDataPoints, ());
  }

  {
    // Case when the distribution is not changed. All mwms lose
    // the same percent of data points. But mwms with small number of points are moved.
    auto distr = distribution;
    auto matched = matchedDataPoints;
    auto const balancedDataPointNumber =
        BalancedDataPointNumber(std::move(matched), std::move(distr), 7 /* ignoreDataPointsNumber */);
    MwmToDataPoints expectedBalancedDataPointNumber = {{"Russia_Moscow", 10 /* data points */}};
    TEST_EQUAL(balancedDataPointNumber, expectedBalancedDataPointNumber, ());
  }
}

UNIT_TEST(BalancedDataPointNumberTest)
{
  MwmToDataPoints distribution = {{"Russia_Moscow", 6000 /* data points */},
                                  {"San Marino", 3000 /* data points */},
                                  {"Slovakia", 1000 /* data points */}};
  MwmToDataPoints matchedDataPoints = {{"Russia_Moscow", 500 /* data points */},
                                       {"San Marino", 300 /* data points */},
                                       {"Slovakia", 10 /* data points */}};
  {
    auto distr = distribution;
    auto matched = matchedDataPoints;
    auto const balancedDataPointNumber =
        BalancedDataPointNumber(std::move(matched), std::move(distr), 7 /* ignoreDataPointsNumber */);
    MwmToDataPoints expectedBalancedDataPointNumber = {{"Russia_Moscow", 60 /* data points */},
                                                       {"San Marino", 30 /* data points */},
                                                       {"Slovakia", 10 /* data points */}};
    TEST_EQUAL(balancedDataPointNumber, expectedBalancedDataPointNumber, ());
  }
}

UNIT_TEST(BalancedDataPointNumberUint64Test)
{
  MwmToDataPoints distribution = {{"Russia_Moscow", 6'000'000'000'000 /* data points */},
                                  {"San Marino", 3'000'000'000'000 /* data points */},
                                  {"Slovakia", 1'000'000'000'000 /* data points */}};
  MwmToDataPoints matchedDataPoints = {{"Russia_Moscow", 500'000'000'000 /* data points */},
                                       {"San Marino", 300'000'000'000 /* data points */},
                                       {"Slovakia", 10'000'000'000 /* data points */}};
  {
    auto distr = distribution;
    auto matched = matchedDataPoints;
    auto const balancedDataPointNumber =
        BalancedDataPointNumber(std::move(matched), std::move(distr), 7'000'000'000 /* ignoreDataPointsNumber */);
    MwmToDataPoints expectedBalancedDataPointNumber = {{"Russia_Moscow", 60'000'000'000 /* data points */},
                                                       {"San Marino", 30'000'000'000 /* data points */},
                                                       {"Slovakia", 10'000'000'000 /* data points */}};
    TEST_EQUAL(balancedDataPointNumber, expectedBalancedDataPointNumber, ());
  }
}

UNIT_TEST(FilterTableTest)
{
  std::stringstream ss;
  std::vector<TableRow> csvTable;
  MwmToDataPoints matchedDataPoints;

  ss << csv;
  FillTable(ss, matchedDataPoints, csvTable);

  auto const calcRecords = [](vector<TableRow> const & t, string const & mwmName)
  {
    return count_if(t.cbegin(), t.cend(),
                    [&mwmName](TableRow const & row) { return row[kMwmNameCsvColumn] == mwmName; });
  };

  {
    auto table = csvTable;
    MwmToDataPoints const balancedDataPointNumbers = {{"New Zealand", 2 /* data points */},
                                                      {"Poland", 1 /* data points */}};
    FilterTable(balancedDataPointNumbers, table);
    TEST_EQUAL(table.size(), 3, ());
    TEST_EQUAL(calcRecords(table, "New Zealand"), 2, ());
    TEST_EQUAL(calcRecords(table, "Poland"), 1, ());
  }

  {
    auto table = csvTable;
    MwmToDataPoints const balancedDataPointNumbers = {{"New Zealand", 1 /* data points */},
                                                      {"Poland", 1 /* data points */}};
    FilterTable(balancedDataPointNumbers, table);
    TEST_EQUAL(table.size(), 2, ());
    TEST_EQUAL(calcRecords(table, "New Zealand"), 1, ());
    TEST_EQUAL(calcRecords(table, "Poland"), 1, ());
  }

  {
    auto table = csvTable;
    MwmToDataPoints const balancedDataPointNumbers = {{"New Zealand", 2 /* data points */}};
    FilterTable(balancedDataPointNumbers, table);
    TEST_EQUAL(table.size(), 2, ());
    TEST_EQUAL(calcRecords(table, "New Zealand"), 2, ());
    TEST_EQUAL(calcRecords(table, "Poland"), 0, ());
  }
}
}  // namespace

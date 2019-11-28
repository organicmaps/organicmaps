#include "track_analyzing/track_analyzer/balance_csv_impl.hpp"

#include "track_analyzing/track_analyzer/utils.hpp"

#include "coding/csv_reader.hpp"

#include "base/math.hpp"

#include <algorithm>
#include <random>
#include <utility>

namespace
{
double constexpr kSumFractionEps = 1e-5;
}  // namespace

namespace track_analyzing
{
using namespace std;

void FillTable(basic_istream<char> & tableCsvStream, MwmToDataPoints & matchedDataPoints,
               vector<TableRow> & table)
{
  for (auto const & row : coding::CSVRunner(
      coding::CSVReader(tableCsvStream, true /* hasHeader */, ',' /* delimiter */)))
  {
    CHECK_EQUAL(row.size(), 19 /* number fields in table csv */, (row));
    auto const & mwmName = row[kMwmNameCsvColumn];
    matchedDataPoints[mwmName] += 1;
    table.push_back(row);
  }
}

void RemoveKeysSmallValue(MwmToDataPoints & checkedMap, MwmToDataPoints & additionalMap,
                          uint32_t ignoreDataPointNumber)
{
  CHECK(AreKeysEqual(additionalMap, checkedMap),
        ("Mwms in |checkedMap| and in |additionalMap| should have the same set of keys."));

  for (auto it = checkedMap.begin() ; it != checkedMap.end();)
  {
    auto const dataPointNumberAfterMatching = it->second;
    if (dataPointNumberAfterMatching > ignoreDataPointNumber)
    {
      ++it;
      continue;
    }

    auto const & mwmName = it->first;
    additionalMap.erase(mwmName);
    it = checkedMap.erase(it);
  }
}

void FillsMwmToDataPointFraction(MwmToDataPoints const & numberMapping, MwmToDataPointFraction & fractionMapping)
{
  CHECK(!numberMapping.empty(), ());
  uint32_t const totalDataPointNumber = ValueSum(numberMapping);

  fractionMapping.clear();
  for (auto const & kv : numberMapping)
    fractionMapping.emplace(kv.first, static_cast<double>(kv.second) / totalDataPointNumber);
}

void CalcsMatchedDataPointsToKeepDistribution(MwmToDataPoints const & matchedDataPoints,
                                              MwmToDataPointFraction const & distributionFractions,
                                              MwmToDataPoints & matchedDataPointsToKeepDistribution)
{
  CHECK(!matchedDataPoints.empty(), ());
  CHECK(AreKeysEqual(matchedDataPoints, distributionFractions),
        ("Mwms in |matchedDataPoints| and in |distributionFractions| should have the same set of keys."));
  CHECK(base::AlmostEqualAbs(ValueSum(distributionFractions), 1.0, kSumFractionEps),
        (ValueSum(distributionFractions)));

  matchedDataPointsToKeepDistribution.clear();

  MwmToDataPointFraction matchedFractions;
  FillsMwmToDataPointFraction(matchedDataPoints, matchedFractions);

  double maxRatio = 0.0;
  double maxRationDistributionFraction = 0.0;
  uint32_t maxRationMatchedDataPointNumber = 0;
  // First, let's find such mwm that all |matchedDataPoints| of it may be used and
  // the distribution set in |distributionFractions| will be kept. It's an mwm
  // on which the maximum of ratio
  // (percent of data points in the distribution) / (percent of matched data points)
  // is reached.
  for (auto const & kv : matchedDataPoints)
  {
    auto const & mwmName = kv.first;
    auto const matchedDataPointNumber = kv.second;
    auto const it = distributionFractions.find(mwmName);
    CHECK(it != distributionFractions.cend(), (mwmName));
    auto const distributionFraction = it->second;
    double const matchedFraction = matchedFractions[mwmName];

    double const ratio = distributionFraction / matchedFraction;
    if (ratio > maxRatio)
    {
      maxRatio = ratio;
      maxRationDistributionFraction = distributionFraction;
      maxRationMatchedDataPointNumber = matchedDataPointNumber;
    }
  }
  CHECK_GREATER_OR_EQUAL(maxRatio, 1.0, ());

  // Having data points number in mwm on which the maximum is reached and
  // fraction of data point in the distribution for this mwm (less or equal 1.0) it's possible to
  // calculate the total matched points number which may be used to keep the distribution.
  auto const totalMatchedPointNumberToKeepDistribution =
      static_cast<uint32_t>(maxRationMatchedDataPointNumber / maxRationDistributionFraction);
  CHECK_LESS_OR_EQUAL(totalMatchedPointNumberToKeepDistribution, ValueSum(matchedDataPoints), ());

  // Having total maximum matched point number which let to keep the distribution
  // and which fraction for each mwm should be used to correspond to the distribution
  // it's possible to find matched data point number which may be used for each mwm
  // to fulfil the distribution.
  for (auto const & kv : distributionFractions)
  {
    auto const fraction = kv.second;
    matchedDataPointsToKeepDistribution.emplace(
        kv.first, static_cast<uint32_t>(fraction * totalMatchedPointNumberToKeepDistribution));
  }
}

void BalanceDataPointNumber(MwmToDataPoints && distribution,
                            MwmToDataPoints && matchedDataPoints, uint32_t ignoreDataPointsNumber,
                            MwmToDataPoints & balancedDataPointNumber)
{
  // Removing every mwm from |distribution| and |matchedDataPoints| if it has
  // |ignoreDataPointsNumber| data points or less in |matchedDataPoints|.
  RemoveKeysSmallValue(matchedDataPoints, distribution, ignoreDataPointsNumber);

  // Calculating how much percent data points in each mwm of |distribution|.
  MwmToDataPointFraction distributionFraction;
  FillsMwmToDataPointFraction(distribution, distributionFraction);

  CHECK(AreKeysEqual(distribution, matchedDataPoints),
        ("Mwms in |checkedMap| and in |additionalMap| should have the same set of keys."));
  // The distribution defined in |distribution| and the distribution after matching
  // may be different. (They are most likely different.) So to keep the distribution
  // defined in |distribution| after matching only a part of matched data points
  // should be used.
  // Filling |balancedDataPointNumber| with information about how many data points
  // form |distributionCsvStream| should be used for every mwm.
  CalcsMatchedDataPointsToKeepDistribution(matchedDataPoints, distributionFraction,
                                           balancedDataPointNumber);
}

void FilterTable(MwmToDataPoints const & balancedDataPointNumbers, vector<TableRow> & table)
{
  map<string, vector<size_t>> tableIdx;
  for (size_t idx = 0; idx < table.size(); ++idx)
  {
    auto const & row = table[idx];
    tableIdx[row[kMwmNameCsvColumn]].push_back(idx);
  }

  // Shuffling data point indexes and reducing the size of index vector for each mwm according to
  // |balancedDataPointNumbers|.
  random_device rd;
  mt19937 g(rd());
  for (auto & kv : tableIdx)
  {
    auto const & mwmName = kv.first;
    auto & mwmRecordsIndices = kv.second;
    auto it = balancedDataPointNumbers.find(mwmName);
    if (it == balancedDataPointNumbers.end())
    {
      mwmRecordsIndices.resize(0); // No indices.
      continue;
    }

    auto const balancedDataPointNumber = it->second;
    CHECK_LESS_OR_EQUAL(balancedDataPointNumber, mwmRecordsIndices.size(), (mwmName));
    shuffle(mwmRecordsIndices.begin(), mwmRecordsIndices.end(), g);
    mwmRecordsIndices.resize(balancedDataPointNumber);
  }

  // Refilling |table| with part of its items to corresponds |balancedDataPointNumbers| distribution.
  vector<TableRow> balancedTable;
  for (auto const & kv : tableIdx)
  {
    for (auto const idx : kv.second)
      balancedTable.emplace_back(move(table[idx]));
  }
  table = move(balancedTable);
}

void BalanceDataPoints(basic_istream<char> & distributionCsvStream,
                       basic_istream<char> & tableCsvStream, uint32_t ignoreDataPointsNumber,
                       vector<TableRow> & balancedTable)
{
  // Filling a map mwm to DataPoints number according to distribution csv file.
  MwmToDataPoints distribution;
  MappingFromCsv(distributionCsvStream, distribution);

  balancedTable.clear();
  MwmToDataPoints matchedDataPoints;
  FillTable(tableCsvStream, matchedDataPoints, balancedTable);

  if (matchedDataPoints.empty())
  {
    CHECK(balancedTable.empty(), ());
    return;
  }

  // Removing every mwm from |distribution| if there is no such mwm in |matchedDataPoints|.
  for (auto it = distribution.begin(); it != distribution.end();)
  {
    if (matchedDataPoints.count(it->first) == 0)
      it = distribution.erase(it);
    else
      ++it;
  }
  CHECK(AreKeysEqual(distribution, matchedDataPoints),
        ("Mwms in |distribution| and in |matchedDataPoints| should have the same set of keys."));

  // Calculating how many points should have every mwm to keep the |distribution|.
  MwmToDataPoints balancedDataPointNumber;
  BalanceDataPointNumber(move(distribution), move(matchedDataPoints), ignoreDataPointsNumber,
                         balancedDataPointNumber);

  // |balancedTable| is filled now with all the items from |tableCsvStream|.
  // Removing some items form |tableCsvStream| (if it's necessary) to correspond to
  // the distribution.
  FilterTable(balancedDataPointNumber, balancedTable);
}
}  // namespace track_analyzing

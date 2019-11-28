#pragma once
#include "track_analyzing/track_analyzer/utils.hpp"

#include <cstdint>
#include <map>
#include <sstream>
#include <string>
#include <vector>

namespace track_analyzing
{
// Mwm to data points number in the mwm mapping.
using MwmToDataPoints = Stats::NameToCountMapping;
// Csv table row.
using TableRow = std::vector<std::string>;
// Mwm to data points percent in the mwm mapping. It's assumed that the sum of the values
// should be equal to 100.0 percents.
using MwmToDataPointFraction = std::map<std::string, double>;

size_t constexpr kMwmNameCsvColumn = 1;

/// \brief Fills a map |mwmToDataPoints| mwm to DataPoints number according to |tableCsvStream| and
/// fills |table| with all the content of |tableCsv|.
void FillTable(std::basic_istream<char> & tableCsvStream, MwmToDataPoints & matchedDataPoints,
               std::vector<TableRow> & table);

/// \brief Removing every mwm from |checkedMap| and |additionalMap| if it has
/// |ignoreDataPointsNumber| data points or less in |checkedMap|.
void RemoveKeysSmallValue(MwmToDataPoints & checkedMap, MwmToDataPoints & additionalMap,
                          uint32_t ignoreDataPointNumber);

/// \brief Fills |fractionMapping|. It is the mapping from mwm to fraction of data points contained
/// in the mwm.
void FillsMwmToDataPointFraction(MwmToDataPoints const & numberMapping,
                                 MwmToDataPointFraction & fractionMapping);

/// \brief Fills mapping |matchedDataPointsToKeepDistribution| fulfilling two conditions:
/// *  number of data points in |matchedDataPointsToKeepDistribution| is less or equal
///    number of data points in |matchedDataPoints| for every mwm
/// * distribution defined by |distributionFraction| is kept
void CalcsMatchedDataPointsToKeepDistribution(MwmToDataPoints const & matchedDataPoints,
                                              MwmToDataPointFraction const & distributionFractions,
                                              MwmToDataPoints & matchedDataPointsToKeepDistribution);

/// \brief Fills |balancedDataPointNumber| with number of matched data points for every mwm
/// to correspond to |distribution|.
void BalanceDataPointNumber(MwmToDataPoints && distribution,
                            MwmToDataPoints && matchedDataPoints, uint32_t ignoreDataPointsNumber,
                            MwmToDataPoints & balancedDataPointNumber);

/// \breif Leaves in |table| only number of items according to |balancedDataPointNumbers|.
/// \note |table| may have a significant size. It may be several tens millions records.
void FilterTable(MwmToDataPoints const & balancedDataPointNumbers, std::vector<TableRow> & table);

/// \brief Fills |balancedTable| with TableRow(s) according to the distribution set in
/// |distributionCsvStream|.
void BalanceDataPoints(std::basic_istream<char> & distributionCsvStream,
                       std::basic_istream<char> & tableCsvStream, uint32_t ignoreDataPointsNumber,
                       std::vector<TableRow> & balancedTable);

template <typename MapCont>
typename MapCont::mapped_type ValueSum(MapCont const & mapCont)
{
  typename MapCont::mapped_type valueSum = typename MapCont::mapped_type();
  for (auto const & kv : mapCont)
    valueSum += kv.second;
  return valueSum;
}

/// \returns true if |map1| and |map2| have the same key and false otherwise.
template<typename Map1, typename Map2>
bool AreKeysEqual(Map1 const & map1, Map2 const & map2)
{
  if (map1.size() != map2.size())
    return false;

  for (auto const & kv : map1)
  {
    if (map2.count(kv.first) == 0)
      return false;
  }
  return true;
}
}  // namespace track_analyzing

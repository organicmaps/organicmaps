#include "track_analyzing/track_analyzer/utils.hpp"

#include "track_analyzing/log_parser.hpp"

#include "storage/country_info_getter.hpp"
#include "storage/routing_helpers.hpp"

#include "geometry/tree4d.hpp"

#include "platform/platform.hpp"

#include "coding/csv_reader.hpp"

#include "base/checked_cast.hpp"
#include "base/logging.hpp"
#include "base/string_utils.hpp"

#include <algorithm>
#include <fstream>
#include <set>
#include <sstream>
#include <utility>

using namespace routing;
using namespace std;
using namespace storage;
using namespace track_analyzing;

namespace
{
set<string> GetKeys(Stats::NameToCountMapping const & mapping)
{
  set<string> keys;
  transform(mapping.begin(), mapping.end(), inserter(keys, keys.end()), [](auto const & kv) { return kv.first; });
  return keys;
}

void Add(Stats::NameToCountMapping const & addition, Stats::NameToCountMapping & base)
{
  set<storage::CountryId> userKeys = GetKeys(addition);
  set<storage::CountryId> const userKeys2 = GetKeys(base);
  userKeys.insert(userKeys2.cbegin(), userKeys2.cend());

  for (auto const & c : userKeys)
  {
    if (addition.count(c) == 0)
      continue;

    if (base.count(c) == 0)
    {
      base[c] = addition.at(c);
      continue;
    }

    base[c] += addition.at(c);
  }
}

void PrintMap(string const & keyName, string const & descr, Stats::NameToCountMapping const & mapping,
              ostringstream & ss)
{
  ss << descr << '\n';
  if (mapping.empty())
  {
    ss << "Map is empty." << endl;
    return;
  }

  MappingToCsv(keyName, mapping, true /* printPercentage */, ss);
}
}  // namespace

namespace track_analyzing
{
// Stats ===========================================================================================
Stats::Stats(NameToCountMapping const & mwmToTotalDataPoints, NameToCountMapping const & countryToTotalDataPoint)
  : m_mwmToTotalDataPoints(mwmToTotalDataPoints)
  , m_countryToTotalDataPoints(countryToTotalDataPoint)
{}

bool Stats::operator==(Stats const & stats) const
{
  return m_mwmToTotalDataPoints == stats.m_mwmToTotalDataPoints &&
         m_countryToTotalDataPoints == stats.m_countryToTotalDataPoints;
}

void Stats::Add(Stats const & stats)
{
  ::Add(stats.m_mwmToTotalDataPoints, m_mwmToTotalDataPoints);
  ::Add(stats.m_countryToTotalDataPoints, m_countryToTotalDataPoints);
}

void Stats::AddTracksStats(MwmToTracks const & mwmToTracks, NumMwmIds const & numMwmIds, Storage const & storage)
{
  for (auto const & kv : mwmToTracks)
  {
    auto const & userToTrack = kv.second;
    uint64_t dataPointNum = 0;
    for (auto const & userTrack : userToTrack)
      dataPointNum += userTrack.second.size();

    NumMwmId const numMwmId = kv.first;
    auto const mwmName = numMwmIds.GetFile(numMwmId).GetName();
    AddDataPoints(mwmName, storage, dataPointNum);
  }
}

void Stats::AddDataPoints(string const & mwmName, string const & countryName, uint64_t dataPointNum)
{
  m_mwmToTotalDataPoints[mwmName] += dataPointNum;
  m_countryToTotalDataPoints[countryName] += dataPointNum;
}

void Stats::AddDataPoints(string const & mwmName, storage::Storage const & storage, uint64_t dataPointNum)
{
  auto const countryName = storage.GetTopmostParentFor(mwmName);
  // Note. In case of disputed mwms |countryName| will be empty.
  AddDataPoints(mwmName, countryName, dataPointNum);
}

void Stats::SaveMwmDistributionToCsv(string const & csvPath) const
{
  LOG(LINFO, ("Saving mwm distribution to", csvPath, "m_mwmToTotalDataPoints size is", m_mwmToTotalDataPoints.size()));
  if (csvPath.empty())
    return;

  ofstream ofs(csvPath);
  CHECK(ofs.is_open(), ("Cannot open file", csvPath));
  MappingToCsv("mwm", m_mwmToTotalDataPoints, false /* printPercentage */, ofs);
}

void Stats::Log() const
{
  LogMwms();
  LogCountries();
}

Stats::NameToCountMapping const & Stats::GetMwmToTotalDataPointsForTesting() const
{
  return m_mwmToTotalDataPoints;
}

Stats::NameToCountMapping const & Stats::GetCountryToTotalDataPointsForTesting() const
{
  return m_countryToTotalDataPoints;
}

void Stats::LogMwms() const
{
  LogNameToCountMapping("mwm", "Mwm to total data points number:", m_mwmToTotalDataPoints);
}

void Stats::LogCountries() const
{
  LogNameToCountMapping("country", "Country name to data points number:", m_countryToTotalDataPoints);
}

void MappingToCsv(string const & keyName, Stats::NameToCountMapping const & mapping, bool printPercentage,
                  basic_ostream<char> & ss)
{
  struct KeyValue
  {
    string m_key;
    uint64_t m_value = 0;
  };

  if (mapping.empty())
    return;

  ss << keyName;
  if (printPercentage)
    ss << ",number,percent\n";
  else
    ss << ",number\n";

  // Sorting from bigger to smaller values.
  vector<KeyValue> keyValues;
  keyValues.reserve(mapping.size());
  for (auto const & kv : mapping)
    keyValues.push_back({kv.first, kv.second});

  sort(keyValues.begin(), keyValues.end(),
       [](KeyValue const & a, KeyValue const & b) { return a.m_value > b.m_value; });

  uint64_t allValues = 0;
  for (auto const & kv : keyValues)
    allValues += kv.m_value;

  for (auto const & kv : keyValues)
  {
    if (kv.m_value == 0)
      continue;

    ss << kv.m_key << "," << kv.m_value;
    if (printPercentage)
      ss << "," << 100.0 * static_cast<double>(kv.m_value) / allValues;
    ss << '\n';
  }
  ss.flush();
}

void MappingFromCsv(basic_istream<char> & ss, Stats::NameToCountMapping & mapping)
{
  for (auto const & row : coding::CSVRunner(coding::CSVReader(ss, true /* hasHeader */, ',' /* kCsvDelimiter */)))
  {
    CHECK_EQUAL(row.size(), 2, (row));
    auto const & key = row[0];
    uint64_t value = 0;
    CHECK(strings::to_uint64(row[1], value), ());
    auto const it = mapping.insert(make_pair(key, value));
    CHECK(it.second, ());
  }
}

void ParseTracks(string const & logFile, shared_ptr<NumMwmIds> const & numMwmIds, MwmToTracks & mwmToTracks)
{
  Platform const & platform = GetPlatform();
  string const dataDir = platform.WritableDir();
  auto countryInfoGetter = CountryInfoReader::CreateCountryInfoGetter(platform);
  unique_ptr<m4::Tree<NumMwmId>> mwmTree = MakeNumMwmTree(*numMwmIds, *countryInfoGetter);

  LOG(LINFO, ("Parsing", logFile));
  LogParser parser(numMwmIds, std::move(mwmTree), dataDir);
  parser.Parse(logFile, mwmToTracks);
}

void WriteCsvTableHeader(basic_ostream<char> & stream)
{
  stream << "user,mwm,hw type,surface type,maxspeed km/h,is city road,is one way,is day,lat lon,"
            "distance,time,mean speed km/h,turn from smaller to bigger,turn from bigger to smaller,"
            "intersection with big\n";
}

void LogNameToCountMapping(string const & keyName, string const & descr, Stats::NameToCountMapping const & mapping)
{
  ostringstream ss;
  LOG(LINFO, ("\n"));
  PrintMap(keyName, descr, mapping, ss);
  LOG(LINFO, (ss.str()));
}

string DebugPrint(Stats const & s)
{
  ostringstream ss;
  ss << "Stats [\n";
  PrintMap("mwm", "Mwm to total data points number:", s.m_mwmToTotalDataPoints, ss);
  PrintMap("country", "Country name to data points number:", s.m_countryToTotalDataPoints, ss);
  ss << "]" << endl;

  return ss.str();
}
}  // namespace track_analyzing

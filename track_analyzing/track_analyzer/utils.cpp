#include "track_analyzing/track_analyzer/utils.hpp"

#include "track_analyzing/log_parser.hpp"

#include "storage/country_info_getter.hpp"
#include "storage/routing_helpers.hpp"

#include "geometry/tree4d.hpp"

#include "platform/platform.hpp"

#include "base/logging.hpp"

#include <algorithm>
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
  transform(mapping.begin(), mapping.end(), inserter(keys, keys.end()),
            [](auto const & kv) { return kv.first; });
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

void AddStat(uint32_t dataPointNum, routing::NumMwmId numMwmId,
             NumMwmIds const & numMwmIds, Storage const & storage, Stats & stats)
{
  auto const mwmName = numMwmIds.GetFile(numMwmId).GetName();
  auto const countryName = storage.GetTopmostParentFor(mwmName);

  // Note. In case of disputed mwms |countryName| will be empty.
  stats.m_mwmToTotalDataPoints[mwmName] += dataPointNum;
  stats.m_countryToTotalDataPoints[countryName] += dataPointNum;
}

void PrintMap(string const & keyType, string const & descr,
              map<string, uint32_t> const & mapping, ostringstream & ss)
{
  struct KeyValue
  {
    string m_key;
    uint32_t m_value = 0;
  };

  ss << descr << '\n';
  if (mapping.empty())
  {
    ss << "Map is empty." << endl;
    return;
  }

  // Sorting from bigger to smaller values.
  vector<KeyValue> keyValues;
  keyValues.reserve(mapping.size());
  for (auto const & kv : mapping)
    keyValues.push_back({kv.first, kv.second});

  sort(keyValues.begin(), keyValues.end(),
            [](KeyValue const & a, KeyValue const & b) { return a.m_value > b.m_value; });

  uint32_t allValues = 0;
  for (auto const & kv : keyValues)
    allValues += kv.m_value;

  ss << keyType << ",number,percent\n";
  for (auto const & kv : keyValues)
  {
    if (kv.m_value == 0)
      continue;

    ss << kv.m_key << "," << kv.m_value << ","
       << 100.0 * static_cast<double>(kv.m_value) / allValues << "\n";
  }
  ss << "\n" << endl;
}
}  // namespace

namespace track_analyzing
{
// Stats ===========================================================================================
void Stats::Add(Stats const & stats)
{
  ::Add(stats.m_mwmToTotalDataPoints, m_mwmToTotalDataPoints);
  ::Add(stats.m_countryToTotalDataPoints, m_countryToTotalDataPoints);
}

bool Stats::operator==(Stats const & stats) const
{
  return m_mwmToTotalDataPoints == stats.m_mwmToTotalDataPoints &&
         m_countryToTotalDataPoints == stats.m_countryToTotalDataPoints;
}

void ParseTracks(string const & logFile, shared_ptr<NumMwmIds> const & numMwmIds,
                 MwmToTracks & mwmToTracks)
{
  Platform const & platform = GetPlatform();
  string const dataDir = platform.WritableDir();
  unique_ptr<CountryInfoGetter> countryInfoGetter =
      CountryInfoReader::CreateCountryInfoReader(platform);
  unique_ptr<m4::Tree<NumMwmId>> mwmTree = MakeNumMwmTree(*numMwmIds, *countryInfoGetter);

  LOG(LINFO, ("Parsing", logFile));
  LogParser parser(numMwmIds, move(mwmTree), dataDir);
  parser.Parse(logFile, mwmToTracks);
}

void AddStat(MwmToTracks const & mwmToTracks, NumMwmIds const & numMwmIds, Storage const & storage,
             Stats & stats)
{
  for (auto const & kv : mwmToTracks)
  {
    auto const & userToTrack = kv.second;
    uint32_t dataPointNum = 0;
    for (auto const & userTrack : userToTrack)
      dataPointNum += userTrack.second.size();

    ::AddStat(dataPointNum, kv.first, numMwmIds, storage, stats);
  }
}

string DebugPrint(Stats const & s)
{
  ostringstream ss;
  ss << "Stats [\n";
  PrintMap("mwm", "Mwm to total data points number:", s.m_mwmToTotalDataPoints, ss);
  PrintMap("country", "Country name to data points number:", s.m_countryToTotalDataPoints, ss);
  ss << "]\n" << endl;

  return ss.str();
}
}  // namespace track_analyzing

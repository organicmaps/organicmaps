#include "geocoder/hierarchy_reader.hpp"

#include "base/logging.hpp"
#include "base/thread_pool_computational.hpp"

#include <algorithm>
#include <iomanip>
#include <list>
#include <sstream>
#include <thread>
#include <vector>

using namespace std;

namespace geocoder
{
namespace
{
// Information will be logged for every |kLogBatch| entries.
size_t const kLogBatch = 100000;

void operator+=(Hierarchy::ParsingStats & accumulator, Hierarchy::ParsingStats & stats)
{
  struct ValidationStats
  {
    uint64_t m_numLoaded, m_badJsons, m_badOsmIds, m_duplicateOsmIds, m_duplicateAddresses,
             m_emptyAddresses, m_emptyNames, m_noLocalityStreets, m_noLocalityBuildings, m_mismatchedNames;
  };
  static_assert(sizeof(Hierarchy::ParsingStats) == sizeof(ValidationStats),
                "Hierarchy::ParsingStats has been modified");

  accumulator.m_numLoaded += stats.m_numLoaded;
  accumulator.m_badJsons += stats.m_badJsons;
  accumulator.m_badOsmIds += stats.m_badOsmIds;
  accumulator.m_duplicateOsmIds += stats.m_duplicateOsmIds;
  accumulator.m_duplicateAddresses += stats.m_duplicateAddresses;
  accumulator.m_emptyAddresses += stats.m_emptyAddresses;
  accumulator.m_emptyNames += stats.m_emptyNames;
  accumulator.m_noLocalityStreets += stats.m_noLocalityStreets;
  accumulator.m_noLocalityBuildings += stats.m_noLocalityBuildings;
  accumulator.m_mismatchedNames += stats.m_mismatchedNames;
}
} // namespace

HierarchyReader::HierarchyReader(string const & pathToJsonHierarchy)
  : m_fileStream{pathToJsonHierarchy}, m_in{m_fileStream}
{
  if (!m_fileStream)
    MYTHROW(OpenException, ("Failed to open file", pathToJsonHierarchy));
}

HierarchyReader::HierarchyReader(istream & in)
  : m_in{in}
{
}

Hierarchy HierarchyReader::Read(unsigned int readersCount)
{
  CHECK_GREATER_OR_EQUAL(readersCount, 1, ());

  LOG(LINFO, ("Reading entries..."));

  vector<Entry> entries;
  NameDictionaryBuilder nameDictionaryBuilder;
  ParsingStats stats{};

  base::thread_pool::computational::ThreadPool threadPool{readersCount};
  list<future<ParsingResult>> tasks{};
  while (!m_eof || !tasks.empty())
  {
    size_t const kReadBlockLineCount = 1000;
    while (!m_eof && tasks.size() <= 2 * readersCount)
      tasks.emplace_back(threadPool.Submit([&] { return ReadEntries(kReadBlockLineCount); }));

    CHECK(!tasks.empty(), ());
    auto & task = tasks.front();
    auto taskResult = task.get();
    tasks.pop_front();

    auto & taskEntries = taskResult.m_entries;
    auto const & taskNameDictionary = taskResult.m_nameDictionary;
    for (auto & entry : taskEntries)
    {
      for (size_t i = 0; i < static_cast<size_t>(Type::Count); ++i)
      {
        if (auto & position = entry.m_normalizedAddress[i])
        {
          auto const & name = taskNameDictionary.Get(position);
          position = nameDictionaryBuilder.Add(name);
        }
      }
    }
    move(begin(taskEntries), end(taskEntries), back_inserter(entries));

    stats += taskResult.m_stats;
  }

  if (m_totalNumLoaded % kLogBatch != 0)
    LOG(LINFO, ("Read", m_totalNumLoaded, "entries"));

  LOG(LINFO, ("Sorting entries..."));
  sort(begin(entries), end(entries));
  LOG(LINFO, ("Finished entries sorting"));

  CheckDuplicateOsmIds(entries, stats);

  LOG(LINFO, ("Finished reading and indexing the hierarchy. Stats:"));
  LOG(LINFO, ("Entries loaded:", stats.m_numLoaded));
  LOG(LINFO, ("Corrupted json lines:", stats.m_badJsons));
  LOG(LINFO, ("Unreadable base::GeoObjectIds:", stats.m_badOsmIds));
  LOG(LINFO, ("Duplicate base::GeoObjectIds:", stats.m_duplicateOsmIds));
  LOG(LINFO, ("Entries with duplicate address parts:", stats.m_duplicateAddresses));
  LOG(LINFO, ("Entries without address:", stats.m_emptyAddresses));
  LOG(LINFO, ("Entries without names:", stats.m_emptyNames));
  LOG(LINFO, ("Street entries without a locality name:", stats.m_noLocalityStreets));
  LOG(LINFO, ("Building entries without a locality name:", stats.m_noLocalityBuildings));
  LOG(LINFO,
      ("Entries whose names do not match their most specific addresses:", stats.m_mismatchedNames));
  LOG(LINFO, ("(End of stats.)"));

  return Hierarchy{move(entries), nameDictionaryBuilder.Release()};
}

void HierarchyReader::CheckDuplicateOsmIds(vector<geocoder::Hierarchy::Entry> const & entries,
                                           ParsingStats & stats)
{
  size_t i = 0;
  while (i < entries.size())
  {
    size_t j = i + 1;
    while (j < entries.size() && entries[i].m_osmId == entries[j].m_osmId)
      ++j;
    if (j != i + 1)
    {
      ++stats.m_duplicateOsmIds;
      LOG(LDEBUG,
          ("Duplicate osm id:", SerializeId(entries[i].m_osmId.GetEncodedId()), "(",
           SerializeId(entries[i].m_osmId.GetEncodedId()), ")", "occurs as a key in",
           j - i, "key-value entries."));
    }
    i = j;
  }
}

HierarchyReader::ParsingResult HierarchyReader::ReadEntries(size_t count)
{
  vector<string> linesBuffer(count);
  size_t bufferSize = 0;

  {
    lock_guard<mutex> lock(m_mutex);

    for (; bufferSize < count; ++bufferSize)
    {
      if (!getline(m_in, linesBuffer[bufferSize]))
      {
        m_eof = true;
        break;
      }
    }
  }

  return DeserializeEntries(linesBuffer, bufferSize);
}

HierarchyReader::ParsingResult HierarchyReader::DeserializeEntries(
    vector<string> const & linesBuffer, size_t const bufferSize)
{
  vector<Entry> entries;
  entries.reserve(bufferSize);
  NameDictionaryBuilder nameDictionaryBuilder;
  ParsingStats stats;

  for (size_t i = 0; i < bufferSize; ++i)
  {
    auto & line = linesBuffer[i];

    if (line.empty())
      continue;

    auto const p = line.find(' ');
    uint64_t encodedId;
    if (p == string::npos || !DeserializeId(line.substr(0, p), encodedId))
    {
      LOG(LWARNING, ("Cannot read osm id. Line:", line));
      ++stats.m_badOsmIds;
      continue;
    }
    auto json = line.substr(p + 1);

    Entry entry;
    auto const osmId = base::GeoObjectId(encodedId);
    entry.m_osmId = osmId;

    if (!entry.DeserializeFromJSON(json, nameDictionaryBuilder, stats))
      continue;

    if (entry.m_type == Type::Count)
      continue;

    ++stats.m_numLoaded;

    auto totalNumLoaded = m_totalNumLoaded.fetch_add(1) + 1;
    if (totalNumLoaded % kLogBatch == 0)
      LOG(LINFO, ("Read", totalNumLoaded, "entries"));

    entries.push_back(move(entry));
  }

  return {move(entries), nameDictionaryBuilder.Release(), move(stats)};
}

// static
bool HierarchyReader::DeserializeId(string const & str, uint64_t & id)
{
  return strings::to_uint64(str, id, 16 /* base */);
}

// static
string HierarchyReader::SerializeId(uint64_t id)
{
  stringstream s;
  s << setw(16) << setfill('0') << hex << uppercase << id;
  return s.str();
}
}  // namespace geocoder

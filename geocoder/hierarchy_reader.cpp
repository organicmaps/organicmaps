#include "geocoder/hierarchy_reader.hpp"

#include "base/logging.hpp"

#include <queue>
#include <thread>

using namespace std;

namespace geocoder
{
namespace
{
// Information will be logged for every |kLogBatch| entries.
size_t const kLogBatch = 100000;

void operator += (Hierarchy::ParsingStats & accumulator, Hierarchy::ParsingStats & stats)
{
  struct ValidationStats
  {
    uint64_t m_numLoaded, m_badJsons, m_badOsmIds, m_duplicateOsmIds, m_duplicateAddresses,
             m_emptyAddresses, m_emptyNames, m_mismatchedNames;
  };
  static_assert(sizeof(Hierarchy::ParsingStats) == sizeof(ValidationStats), "");

  accumulator.m_numLoaded           += stats.m_numLoaded;
  accumulator.m_badJsons            += stats.m_badJsons;
  accumulator.m_badOsmIds           += stats.m_badOsmIds;
  accumulator.m_duplicateOsmIds     += stats.m_duplicateOsmIds;
  accumulator.m_duplicateAddresses  += stats.m_duplicateAddresses;
  accumulator.m_emptyAddresses      += stats.m_emptyAddresses;
  accumulator.m_emptyNames          += stats.m_emptyNames;
  accumulator.m_mismatchedNames     += stats.m_mismatchedNames;
}

} // namespace

HierarchyReader::HierarchyReader(string const & pathToJsonHierarchy)
  : m_fileStream{pathToJsonHierarchy}, m_in{m_fileStream}
{
  if (!m_fileStream)
    MYTHROW(OpenException, ("Failed to open file", pathToJsonHierarchy));
}

HierarchyReader::HierarchyReader(std::istream & in)
  : m_in{in}
{
}

Hierarchy HierarchyReader::Read(size_t readersCount)
{
  LOG(LINFO, ("Reading entries..."));

  readersCount = min(readersCount, size_t{thread::hardware_concurrency()});

  vector<multimap<base::GeoObjectId, Entry>> taskEntries(readersCount);
  vector<ParsingStats> tasksStats(readersCount);
  vector<thread> tasks{};
  for (size_t t = 0; t < readersCount; ++t)
    tasks.emplace_back(&HierarchyReader::ReadEntryMap, this, ref(taskEntries[t]), ref(tasksStats[t]));

  for (auto & reader : tasks)
    reader.join();

  if (m_totalNumLoaded % kLogBatch != 0)
    LOG(LINFO, ("Read", m_totalNumLoaded, "entries"));

  ParsingStats stats{};
  for (auto & readerStats : tasksStats)
    stats += readerStats;

  auto entries = MergeEntries(taskEntries);

  CheckDuplicateOsmIds(entries, stats);

  LOG(LINFO, ("Finished reading and indexing the hierarchy. Stats:"));
  LOG(LINFO, ("Entries loaded:", stats.m_numLoaded));
  LOG(LINFO, ("Corrupted json lines:", stats.m_badJsons));
  LOG(LINFO, ("Unreadable base::GeoObjectIds:", stats.m_badOsmIds));
  LOG(LINFO, ("Duplicate base::GeoObjectIds:", stats.m_duplicateOsmIds));
  LOG(LINFO, ("Entries with duplicate address parts:", stats.m_duplicateAddresses));
  LOG(LINFO, ("Entries without address:", stats.m_emptyAddresses));
  LOG(LINFO, ("Entries without names:", stats.m_emptyNames));
  LOG(LINFO,
      ("Entries whose names do not match their most specific addresses:", stats.m_mismatchedNames));
  LOG(LINFO, ("(End of stats.)"));

  return Hierarchy{std::move(entries), true};
}

vector<Hierarchy::Entry> HierarchyReader::MergeEntries(vector<multimap<base::GeoObjectId, Entry>> & entryParts)
{
  auto entries = vector<Entry>{};

  size_t size{0};
  for (auto const & map : entryParts)
    size += map.size();

  entries.reserve(size);

  LOG(LINFO, ("Merging entries..."));

  using PartReference = reference_wrapper<multimap<base::GeoObjectId, Entry>>;
  struct ReferenceGreater
  {
    bool operator () (PartReference const & l, PartReference const & r) const noexcept
    { return l.get() > r.get(); }
  };

  auto partsQueue = priority_queue<PartReference, std::vector<PartReference>, ReferenceGreater>
                      (entryParts.begin(), entryParts.end());
  while (partsQueue.size())
  {
    auto & minPart = partsQueue.top().get();
    partsQueue.pop();

    while (minPart.size() && (partsQueue.empty() || minPart <= partsQueue.top().get()))
    {
      entries.emplace_back(move(minPart.begin()->second));
      minPart.erase(minPart.begin());
    }

    if (minPart.size())
      partsQueue.push(ref(minPart));
  }

  return entries;
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
      // todo Remove the cast when the hierarchies no longer contain negative keys.
      LOG(LDEBUG,
          ("Duplicate osm id:", static_cast<int64_t>(entries[i].m_osmId.GetEncodedId()), "(",
           entries[i].m_osmId, ")", "occurs as a key in", j - i, "key-value entries."));
    }
    i = j;
  }
}

void HierarchyReader::ReadEntryMap(multimap<base::GeoObjectId, Entry> & entries, ParsingStats & stats)
{
  // Temporary local object for efficient concurent processing (individual cache line for container).
  auto localEntries = multimap<base::GeoObjectId, Entry>{};

  int const kLineBufferCapacity = 10000;
  vector<string> linesBuffer(kLineBufferCapacity);
  int bufferSize = 0;

  while (true)
  {
    bufferSize = 0;

    {
      auto && lock = lock_guard<mutex>(m_mutex);

      for (; bufferSize < kLineBufferCapacity; ++bufferSize)
      {
        if (!getline(m_in, linesBuffer[bufferSize]))
          break;
      }
    }

    if (!bufferSize)
      break;

    DeserializeEntryMap(linesBuffer, bufferSize, localEntries, stats);
  }

  entries = move(localEntries);
}

void HierarchyReader::DeserializeEntryMap(vector<string> const & linesBuffer, int const bufferSize, 
  multimap<base::GeoObjectId, Entry> & entries, ParsingStats & stats)
{
  for (int i = 0; i < bufferSize; ++i)
  {
    auto & line = linesBuffer[i];

    if (line.empty())
      continue;

    auto const p = line.find(' ');
    int64_t encodedId;
    if (p == string::npos || !strings::to_any(line.substr(0, p), encodedId))
    {
      LOG(LWARNING, ("Cannot read osm id. Line:", line));
      ++stats.m_badOsmIds;
      continue;
    }
    auto json = line.substr(p + 1);

    Entry entry;
    // todo(@m) We should really write uints as uints.
    auto const osmId = base::GeoObjectId(static_cast<uint64_t>(encodedId));
    entry.m_osmId = osmId;

    if (!entry.DeserializeFromJSON(json, stats))
      continue;

    if (entry.m_type == Type::Count)
      continue;

    ++stats.m_numLoaded;

    auto totalNumLoaded = m_totalNumLoaded.fetch_add(1) + 1;
    if (totalNumLoaded % kLogBatch == 0)
      LOG(LINFO, ("Read", totalNumLoaded, "entries"));

    entries.emplace(osmId, move(entry));
  }
}
} // namespace geocoder

#include "geocoder/hierarchy_reader.hpp"

#include "base/logging.hpp"
#include "base/string_utils.hpp"

#include <boost/iostreams/device/file.hpp>
#include <boost/iostreams/filter/gzip.hpp>

#include <queue>
#include <thread>

using namespace std;

namespace geocoder
{
namespace
{
// Information will be logged for every |kLogBatch| entries.
size_t const kLogBatch = 100000;
} // namespace

HierarchyReader::HierarchyReader(string const & pathToJsonHierarchy)
{
  using namespace boost::iostreams;

  if (strings::EndsWith(pathToJsonHierarchy, ".gz"))
    m_fileStream.push(gzip_decompressor());
  m_fileStream.push(file_source(pathToJsonHierarchy));

  if (!m_fileStream)
    MYTHROW(OpenException, ("Failed to open file", pathToJsonHierarchy));
}

vector<Hierarchy::Entry> HierarchyReader::ReadEntries(size_t readersCount, ParsingStats & stats)
{
  LOG(LINFO, ("Reading entries..."));

  readersCount = min(readersCount, size_t{thread::hardware_concurrency()});

  vector<multimap<base::GeoObjectId, Entry>> taskEntries(readersCount);
  vector<thread> tasks{};
  for (size_t t = 0; t < readersCount; ++t)
    tasks.emplace_back(&HierarchyReader::ReadEntryMap, this, ref(taskEntries[t]), ref(stats));

  for (auto & reader : tasks)
    reader.join();

  if (stats.m_numLoaded % kLogBatch != 0)
    LOG(LINFO, ("Read", stats.m_numLoaded, "entries"));

  return MergeEntries(taskEntries);
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
        if (!getline(m_fileStream, linesBuffer[bufferSize]))
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

    auto numLoaded = stats.m_numLoaded.fetch_add(1) + 1;

    if (numLoaded % kLogBatch == 0)
      LOG(LINFO, ("Read", numLoaded, "entries"));

    entries.emplace(osmId, move(entry));
  }
}
} // namespace geocoder

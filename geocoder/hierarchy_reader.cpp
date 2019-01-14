#include "geocoder/hierarchy_reader.hpp"

#include "base/logging.hpp"

#include <thread>

using namespace std;

namespace geocoder {

namespace
{
// Information will be logged for every |kLogBatch| entries.
size_t const kLogBatch = 100000;
} // namespace

HierarchyReader::HierarchyReader(string const & pathToJsonHierarchy) :
  m_fileStm{pathToJsonHierarchy}
{
  if (!m_fileStm)
    throw runtime_error("failed to open file " + pathToJsonHierarchy);
}

auto HierarchyReader::ReadEntries(size_t readerCount, ParsingStats & stats)
  -> vector<Entry>
{
  LOG(LINFO, ("Reading entries..."));
        
  auto taskEntries = vector<multimap<base::GeoObjectId, Entry>>(readerCount);
  auto tasks = vector<thread>{};
  for (auto t = size_t{0}; t < readerCount; ++t)
    tasks.emplace_back(&HierarchyReader::ReadEntryMap, this, ref(taskEntries[t]), ref(stats));

  for (auto & reader : tasks)
    reader.join();

  if (stats.m_numLoaded % kLogBatch != 0)
    LOG(LINFO, ("Read", stats.m_numLoaded, "entries"));

  return UnionEntries(taskEntries);
}

auto HierarchyReader::UnionEntries(vector<multimap<base::GeoObjectId, Entry>> & entryParts) -> vector<Entry>
{
  auto entries = vector<Entry>{};

  auto size = size_t{0};
  for (auto const & map : entryParts)
    size += map.size();

  entries.reserve(size);

  LOG(LINFO, ("Sorting entries..."));

  while (entryParts.size())
  {
    auto minPart = min_element(entryParts.begin(), entryParts.end());

    entries.emplace_back(std::move(minPart->begin()->second));
    
    minPart->erase(minPart->begin());
    if (minPart->empty())
      entryParts.erase(minPart);
  }

  return entries;
}

void HierarchyReader::ReadEntryMap(multimap<base::GeoObjectId, Entry> & entries, ParsingStats & stats)
{
  // Temporary local object for efficient concurent processing (individual cache line for container).
  auto localEntries = multimap<base::GeoObjectId, Entry>{};

  string line;
  while (true)
  {
    {
      auto && lock = lock_guard<mutex>(m_mutex);
  
      if (!getline(m_fileStm, line))
        break;
    }
        
    if (line.empty())
      continue;

    auto const i = line.find(' ');
    int64_t encodedId;
    if (i == string::npos || !strings::to_any(line.substr(0, i), encodedId))
    {
      LOG(LWARNING, ("Cannot read osm id. Line:", line));
      ++stats.m_badOsmIds;
      continue;
    }
    line = line.substr(i + 1);

    Entry entry;
    // todo(@m) We should really write uints as uints.
    auto osmId = base::GeoObjectId(static_cast<uint64_t>(encodedId));
    entry.m_osmId = osmId;

    if (!entry.DeserializeFromJSON(line, stats))
      continue;

    if (entry.m_type == Type::Count)
      continue;

    ++stats.m_numLoaded;
    if (stats.m_numLoaded % kLogBatch == 0)
      LOG(LINFO, ("Read", (stats.m_numLoaded / kLogBatch) * kLogBatch, "entries"));

    localEntries.emplace(osmId, move(entry));
  }

  entries = move(localEntries);
}

} // namespace geocoder

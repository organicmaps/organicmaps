#pragma once

#include "geocoder/hierarchy.hpp"

#include "base/exception.hpp"
#include "base/geo_object_id.hpp"

#include <atomic>
#include <fstream>
#include <map>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

namespace geocoder
{
class HierarchyReader
{
public:
  using Entry = Hierarchy::Entry;
  using ParsingStats = Hierarchy::ParsingStats;

  DECLARE_EXCEPTION(OpenException, RootException);

  explicit HierarchyReader(std::string const & pathToJsonHierarchy);
  explicit HierarchyReader(std::istream & jsonHierarchy);

  // Read hierarchy file/stream concurrently in |readersCount| threads.
  Hierarchy Read(unsigned int readersCount = 1);

private:
  void ReadEntryMap(std::multimap<base::GeoObjectId, Entry> & entries, ParsingStats & stats);

  void DeserializeEntryMap(std::vector<std::string> const & linesBuffer, std::size_t const bufferSize,
                           std::multimap<base::GeoObjectId, Entry> & entries, ParsingStats & stats);

  std::vector<Entry> MergeEntries(std::vector<std::multimap<base::GeoObjectId, Entry>> & entryParts);
  void CheckDuplicateOsmIds(std::vector<Entry> const & entries, ParsingStats & stats);

  std::ifstream m_fileStream;
  std::istream & m_in;
  std::mutex m_mutex;
  std::atomic<std::uint64_t> m_totalNumLoaded{0};
};
} // namespace geocoder

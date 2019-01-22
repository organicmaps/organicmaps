#pragma once

#include "geocoder/hierarchy.hpp"

#include "base/exception.hpp"
#include "base/geo_object_id.hpp"

#include <fstream>
#include <map>
#include <mutex>
#include <string>
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

  std::vector<Entry> ReadEntries(size_t readersCount, ParsingStats & stats);

private:
  void ReadEntryMap(std::multimap<base::GeoObjectId, Entry> & entries, ParsingStats & stats);
  std::vector<Entry> UnionEntries(std::vector<std::multimap<base::GeoObjectId, Entry>> & entryParts);

  std::ifstream m_fileStm;
  std::mutex m_mutex;
};
} // namespace geocoder

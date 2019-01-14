#pragma once

#include "geocoder/hierarchy.hpp"

#include <fstream>
#include <map>
#include <mutex>
#include <vector>

namespace geocoder
{

class HierarchyReader
{
public:
  using Entry = Hierarchy::Entry;
  using ParsingStats = Hierarchy::ParsingStats;

  HierarchyReader(std::string const & pathToJsonHierarchy);

  auto ReadEntries(size_t readerCount, ParsingStats & stats) -> std::vector<Entry>;

private:
  void ReadEntryMap(std::multimap<base::GeoObjectId, Entry> & entries, ParsingStats & stats);
  auto UnionEntries(std::vector<std::multimap<base::GeoObjectId, Entry>> & entryParts) -> std::vector<Entry>;

  std::ifstream m_fileStm;
  std::mutex m_mutex;
};

} // namespace geocoder

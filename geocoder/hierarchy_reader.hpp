#pragma once

#include "geocoder/hierarchy.hpp"

#include "base/exception.hpp"

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

  DECLARE_EXCEPTION(OpenException, RootException);

  HierarchyReader(std::string const & pathToJsonHierarchy);

  std::vector<Entry> ReadEntries(size_t readerCount, ParsingStats & stats);

private:
  void ReadEntryMap(std::multimap<base::GeoObjectId, Entry> & entries, ParsingStats & stats);
  std::vector<Entry> UnionEntries(std::vector<std::multimap<base::GeoObjectId, Entry>> & entryParts);

  std::ifstream m_fileStm;
  std::mutex m_mutex;
};
} // namespace geocoder

#pragma once

#include "geocoder/hierarchy.hpp"

#include "base/exception.hpp"
#include "base/geo_object_id.hpp"

#include <boost/iostreams/filtering_stream.hpp>

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

  void DeserializeEntryMap(std::vector<std::string> const & linesBuffer, int const bufferSize,
                           std::multimap<base::GeoObjectId, Entry> & entries, ParsingStats & stats);
  std::vector<Entry> MergeEntries(std::vector<std::multimap<base::GeoObjectId, Entry>> & entryParts);

  boost::iostreams::filtering_istream m_fileStream;
  std::mutex m_mutex;
};
} // namespace geocoder

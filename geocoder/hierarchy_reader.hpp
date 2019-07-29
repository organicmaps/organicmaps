#pragma once

#include "geocoder/hierarchy.hpp"
#include "geocoder/name_dictionary.hpp"

#include "base/exception.hpp"
#include "base/geo_object_id.hpp"

#include <atomic>
#include <fstream>
#include <mutex>
#include <string>
#include <thread>
#include <utility>
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
  struct ParsingResult
  {
    std::vector<Entry> m_entries;
    NameDictionary m_nameDictionary;
    ParsingStats m_stats;
  };

  ParsingResult ReadEntries(size_t count);
  ParsingResult DeserializeEntries(std::vector<std::string> const & linesBuffer,
                                   std::size_t const bufferSize);
  static bool DeserializeId(std::string const & str, uint64_t & id);
  static std::string SerializeId(uint64_t id);

  void CheckDuplicateOsmIds(std::vector<Entry> const & entries, ParsingStats & stats);

  std::ifstream m_fileStream;
  std::istream & m_in;
  bool m_eof{false};
  std::mutex m_mutex;
  std::atomic<std::uint64_t> m_totalNumLoaded{0};
};
} // namespace geocoder

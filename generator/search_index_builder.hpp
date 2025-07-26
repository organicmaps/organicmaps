#pragma once

#include "generator/generate_info.hpp"

#include "indexer/ftypes_matcher.hpp"

#include <string>
#include <unordered_map>
#include <vector>

namespace indexer
{
class SynonymsHolder
{
public:
  // Default ctor is reading data/synonyms.txt file.
  SynonymsHolder();
  explicit SynonymsHolder(std::string const & fPath);

  template <class ToDo>
  void ForEach(std::string const & key, ToDo && toDo) const
  {
    auto it = m_map.find(key);
    if (it != m_map.end())
      for (auto const & v : it->second)
        toDo(v);
  }

  template <class Types>
  static bool CanApply(Types const & types)
  {
    auto const type = ftypes::IsLocalityChecker::Instance().GetType(types);
    return (type == ftypes::LocalityType::Country || type == ftypes::LocalityType::State);
  }

private:
  std::unordered_map<std::string, std::vector<std::string>> m_map;
};

// Builds the latest version of the search index section and writes it to the mwm file.
// An attempt to rewrite the search index of an old mwm may result in a future crash
// when using search because this function does not update mwm's version. This results
// in version mismatch when trying to read the index.
bool BuildSearchIndexFromDataFile(std::string const & country, feature::GenerateInfo const & info, bool forceRebuild,
                                  uint32_t threadsCount);
}  // namespace indexer

#pragma once

#include <list>
#include <string>
#include <utility>
#include <vector>

namespace search
{
/// Saves last search queries for search suggestion.
class QuerySaver
{
public:
  /// Search request <locale, request>.
  using TSearchRequest = std::pair<std::string, std::string>;

  QuerySaver();

  void Add(TSearchRequest const & query);

  /// Returns several last saved queries from newest to oldest query.
  /// @see kMaxSuggestCount in implementation file.
  std::list<TSearchRequest> const & Get() const { return m_topQueries; }

  /// Clear last queries storage. All data will be lost.
  void Clear();

private:
  friend void UnitTest_QuerySaverSerializerTest();
  friend void UnitTest_QuerySaverCorruptedStringTest();
  void Serialize(std::string & data) const;
  void Deserialize(std::string const & data);

  void Save();
  void Load();

  std::list<TSearchRequest> m_topQueries;
};
}  // namespace search

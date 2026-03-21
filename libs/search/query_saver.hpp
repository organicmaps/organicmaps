#pragma once

#include <list>
#include <string>

namespace query_saver_tests
{
void UnitTest_QuerySaverSerializerTest();
void UnitTest_QuerySaverCorruptedStringTest();
}  // namespace query_saver_tests

namespace search
{
/// Saves last search queries for search suggestion.
class QuerySaver
{
public:
  /// Search request <locale, request>.
  using SearchRequest = std::pair<std::string, std::string>;

  QuerySaver();

  // TODO(AB): Important category or not flag is not saved.
  void Add(SearchRequest const & query);

  /// Returns several last saved queries from newest to oldest query.
  /// @see kMaxSuggestionsCount in implementation file.
  std::list<SearchRequest> const & Get() const { return m_topQueries; }

  /// Clear last queries storage. All data will be lost.
  void Clear();

private:
  friend void query_saver_tests::UnitTest_QuerySaverSerializerTest();
  friend void query_saver_tests::UnitTest_QuerySaverCorruptedStringTest();
  void Serialize(std::string & data) const;
  void Deserialize(std::string const & data);

  void Save();
  void Load();

  std::list<SearchRequest> m_topQueries;
};
}  // namespace search

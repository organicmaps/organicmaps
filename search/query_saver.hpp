#pragma once

#include "std/list.hpp"
#include "std/string.hpp"
#include "std/vector.hpp"
#include "std/utility.hpp"

namespace search
{
/// Saves last search queries for search suggestion.
class QuerySaver
{
public:
  /// Search request <locale, request>.
  using TSearchRequest = pair<string, string>;
  QuerySaver();
  void Add(TSearchRequest const & query);
  /// Returns several last saved queries from newest to oldest query.
  /// @see kMaxSuggestCount in implementation file.
  list<TSearchRequest> const & Get() const { return m_topQueries; }
  /// Clear last queries storage. All data will be lost.
  void Clear();

private:
  friend void UnitTest_QuerySaverSerializerTest();
  friend void UnitTest_QuerySaverCorruptedStringTest();
  void Serialize(string & data) const;
  void Deserialize(string const & data);

  void Save();
  void Load();

  list<TSearchRequest> m_topQueries;
};
}  // namespace search

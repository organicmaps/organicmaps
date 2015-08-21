#include "std/list.hpp"
#include "std/string.hpp"
#include "std/vector.hpp"

namespace search
{
/// Saves last search queries for search suggestion.
class QuerySaver
{
public:
  QuerySaver();
  void SaveNewQuery(string const & query);
  /// Returns several last saved queries from newest to oldest query.
  /// @see kMaxSuggestCount in implementation file.
  list<string> const & GetTopQueries() const { return m_topQueries; }
  /// Clear last queries storage. All data will be lost.
  void Clear();

private:
  friend void UnitTest_QuerySaverSerializerTest();
  void Serialize(vector<char> & data) const;
  void Deserialize(string const & data);

  void Save();
  void Load();

  list<string> m_topQueries;
};
}  // namespace seatch

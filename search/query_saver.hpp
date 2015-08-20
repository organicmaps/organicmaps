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
  /// Returns last save query from oldest to newest queries.
  vector<string> const & GetTopQueries() const { return m_topQueries; }
  /// Clear last queries storage. All data will be lost.
  void Clear();

private:
  friend void UnitTest_QuerySaverSerializerTest();
  void Serialize(vector<char> & data) const;
  void Deserialize(string const & data);

  void Save();
  void Load();

  vector<string> m_topQueries;
};
}  // namespace seatch

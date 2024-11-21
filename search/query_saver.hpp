#pragma once

#include <list>
#include <string>
#include <chrono>
#include <ostream>

namespace search
{
/// Saves last search queries for search suggestion.
class QuerySaver
{
public:
  /// Search request <locale, request, last access time>.
  struct SearchRequest
  {
    std::string first;
    std::string second;
    std::chrono::system_clock::time_point m_lastAccess;

    bool operator==(SearchRequest const & other) const
    {
      return first == other.first && second == other.second;
    }
  };

  QuerySaver();

  void Add(SearchRequest query);

  /// Returns several last saved queries from newest to oldest query.
  /// @see kMaxSuggestionsCount in implementation file.
  std::list<SearchRequest> Get() const;

  /// Clear last queries storage. All data will be lost.
  void Clear();

private:
  friend void UnitTest_QuerySaverSerializerTest();
  friend void UnitTest_QuerySaverCorruptedStringTest();
  void Serialize(std::string & data) const;
  void Deserialize(std::string const & data);

  void Save();
  void Load();

  std::list<SearchRequest> m_topQueries;
};

/// Overload operator<< for SearchRequest
inline std::ostream & operator<<(std::ostream & os, QuerySaver::SearchRequest const & req)
{
  os << "Locale: " << req.first << ", Query: " << req.second << ", Last Access: " << std::chrono::duration_cast<std::chrono::seconds>(req.m_lastAccess.time_since_epoch()).count();
  return os;
}

}  // namespace search

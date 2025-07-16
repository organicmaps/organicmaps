#pragma once

#include "storage/storage_defines.hpp"

#include <functional>
#include <string>
#include <vector>

namespace storage
{
struct DownloaderSearchResult
{
  DownloaderSearchResult(CountryId const & countryId, std::string const & matchedName)
    : m_countryId(countryId)
    , m_matchedName(matchedName)
  {}

  bool operator==(DownloaderSearchResult const & rhs) const
  {
    return m_countryId == rhs.m_countryId && m_matchedName == rhs.m_matchedName;
  }

  bool operator<(DownloaderSearchResult const & rhs) const
  {
    if (m_countryId != rhs.m_countryId)
      return m_countryId < rhs.m_countryId;
    return m_matchedName < rhs.m_matchedName;
  }

  CountryId m_countryId;
  std::string m_matchedName;
};

struct DownloaderSearchResults
{
  DownloaderSearchResults() : m_endMarker(false) {}

  std::vector<DownloaderSearchResult> m_results;
  std::string m_query;
  // |m_endMarker| is true iff it's the last call of OnResults callback for the search.
  bool m_endMarker;
};

struct DownloaderSearchParams
{
  std::string m_query;
  std::string m_inputLocale;

  using OnResults = std::function<void(DownloaderSearchResults)>;
  OnResults m_onResults;
};

inline std::string DebugPrint(DownloaderSearchResult const & r)
{
  return "(" + r.m_countryId + " " + r.m_matchedName + ")";
}
}  // namespace storage

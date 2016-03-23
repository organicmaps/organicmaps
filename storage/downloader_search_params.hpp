#pragma once

#include "storage/index.hpp"

#include "std/function.hpp"
#include "std/string.hpp"
#include "std/vector.hpp"

namespace storage
{
struct DownloaderSearchResult
{
  DownloaderSearchResult(TCountryId const & countryId, string const & matchedName)
    : m_countryId(countryId), m_matchedName(matchedName)
  {
  }

  TCountryId m_countryId;
  /// \brief |m_matchedName| is a name of found feature in case of searching in World.mwm
  /// and is a local name of mwm (group or leaf) in case of searching in country tree.
  string m_matchedName;
};

struct DownloaderSearchResults
{
  DownloaderSearchResults() : m_endMarker(false) {}

  vector<DownloaderSearchResult> m_results;
  string m_query;
  /// \brief |m_endMarker| == true if it's the last call of TOnResults callback for the search.
  /// Otherwise |m_endMarker| == false.
  bool m_endMarker;
};

struct DownloaderSearchParams
{
  using TOnResults = function<void (DownloaderSearchResults const &)>;

  TOnResults m_onResults;
  string m_query;
  string m_inputLocale;
};
}  // namespace storage

#pragma once

#include "search/result.hpp"
#include "search/search_params.hpp"

#include "base/logging.hpp"

namespace search
{
namespace bookmarks
{
struct Result;
}

class Emitter
{
public:
  void Init(SearchParams::OnResults onResults)
  {
    m_onResults = onResults;
    m_results.Clear();
  }

  bool AddResult(Result && res) { return m_results.AddResult(move(res)); }
  void AddResultNoChecks(Result && res) { m_results.AddResultNoChecks(move(res)); }

  void AddBookmarkResult(bookmarks::Result const & result) { m_results.AddBookmarkResult(result); }

  void Emit()
  {
    if (m_onResults)
      m_onResults(m_results);
    else
      LOG(LERROR, ("OnResults is not set."));
  }

  Results const & GetResults() const { return m_results; }

  void Finish(bool cancelled)
  {
    m_results.SetEndMarker(cancelled);
    if (m_onResults)
      m_onResults(m_results);
    else
      LOG(LERROR, ("OnResults is not set."));
  }

private:
  SearchParams::OnResults m_onResults;
  Results m_results;
};
}  // namespace search

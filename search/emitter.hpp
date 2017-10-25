#pragma once

#include "search/result.hpp"
#include "search/search_params.hpp"

#include "base/logging.hpp"

namespace search
{
class Emitter
{
public:
  inline void Init(SearchParams::OnResults onResults)
  {
    m_onResults = onResults;
    m_results.Clear();
  }

  inline bool AddResult(Result && res) { return m_results.AddResult(move(res)); }
  inline void AddResultNoChecks(Result && res) { m_results.AddResultNoChecks(move(res)); }

  inline void Emit()
  {
    if (m_onResults)
      m_onResults(m_results);
    else
      LOG(LERROR, ("OnResults is not set."));
  }

  inline Results const & GetResults() const { return m_results; }

  inline void Finish(bool cancelled)
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

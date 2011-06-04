#pragma once

#include "engine.hpp"
#include "intermediate_result.hpp"
#include "keyword_matcher.hpp"
#include "result.hpp"
#include "../geometry/rect2d.hpp"
#include "../base/string_utils.hpp"
#include "../std/function.hpp"
#include "../std/queue.hpp"
#include "../std/scoped_ptr.hpp"
#include "../std/string.hpp"
#include "../std/vector.hpp"

namespace search
{
namespace impl
{

class Query
{
public:
  typedef Engine::IndexType IndexType;

  Query(string const & query, m2::RectD const & viewport, IndexType const * pIndex);

  // Search with parameters, passed in constructor.
  void Search(function<void (Result const &)> const & f);

  // Search and delete this.
  void SearchAndDestroy(function<void (Result const &)> const & f);

  // Add result for scoring.
  void AddResult(IntermediateResult const & result);

  // Set a flag that query is not active any more and should terminate.
  void SetTerminateFlag() volatile { m_bTerminate = true; }

  m2::RectD const & GetViewport() const { return m_viewport; }
  vector<strings::UniString> const & GetKeywords() const { return m_keywords; }
  strings::UniString const & GetPrefix() const { return m_prefix; }

private:
  string m_queryText;
  m2::RectD m_viewport;

  vector<strings::UniString> m_keywords;
  strings::UniString m_prefix;

  scoped_ptr<IndexType const> m_pIndex;
  IndexType::Query m_indexQuery;

  priority_queue<IntermediateResult> m_results;

  bool volatile m_bTerminate;
};

}  // namespace search::impl
}  // namespace search

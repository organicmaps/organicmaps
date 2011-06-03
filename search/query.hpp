#pragma once

#include "engine.hpp"
#include "intermediate_result.hpp"
#include "keyword_matcher.hpp"
#include "result.hpp"
#include "../geometry/rect2d.hpp"
#include "../base/string_utils.hpp"
#include "../std/function.hpp"
#include "../std/queue.hpp"
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

  Query(string const & query, m2::RectD const & rect, IndexType const * pIndex);

  // Search with parameters, passed in constructor.
  void Search(function<void (Result const &)> const & f);

  // Add result for scoring.
  void AddResult(IntermediateResult const & result);

  string m_queryText;
  vector<strings::UniString> m_keywords;
  strings::UniString m_prefix;

  m2::RectD m_rect;

  IndexType const * m_pIndex;
  IndexType::Query m_indexQuery;

  priority_queue<IntermediateResult> m_results;
};

}  // namespace search::impl
}  // namespace search

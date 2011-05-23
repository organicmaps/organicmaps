#pragma once

#include "engine.hpp"
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
  void AddResult(Result const & result);

  struct ResultBetter
  {
    bool operator() (Result const & r1, Result const & r2) const;
  };

  string m_queryText;
  vector<strings::UniString> m_keywords;
  strings::UniString m_prefix;

  m2::RectD m_rect;

  IndexType const * m_pIndex;
  IndexType::Query m_indexQuery;

  priority_queue<Result, vector<Result>, ResultBetter> m_resuts;
};

}  // namespace search::impl
}  // namespace search

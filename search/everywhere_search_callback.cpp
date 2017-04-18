#include "search/everywhere_search_callback.hpp"

#include <cstddef>
#include <utility>

namespace search
{
EverywhereSearchCallback::EverywhereSearchCallback(Delegate & delegate, OnResults onResults)
  : m_delegate(delegate), m_onResults(std::move(onResults))
{
}

void EverywhereSearchCallback::operator()(Results const & results)
{
  auto const prevSize = m_results.GetCount();
  ASSERT_LESS_OR_EQUAL(prevSize, results.GetCount(), ());

  m_results.AddResultsNoChecks(results.begin() + prevSize, results.end());

  for (size_t i = prevSize; i < m_results.GetCount(); ++i)
  {
    if (m_results[i].IsSuggest() ||
        m_results[i].GetResultType() != search::Result::ResultType::RESULT_FEATURE)
    {
      continue;
    }

    m_delegate.MarkLocalAdsCustomer(m_results[i]);
  }

  m_onResults(m_results);
}
}  // namespace search

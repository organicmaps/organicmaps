#include "map/everywhere_search_callback.hpp"

namespace search
{
EverywhereSearchCallback::EverywhereSearchCallback(Delegate & delegate, OnResults onResults)
  : m_delegate(delegate)
  , m_onResults(std::move(onResults))
{
  CHECK(m_onResults, ());
}

void EverywhereSearchCallback::operator()(Results const & results)
{
  size_t const prevSize = m_productInfo.size();
  size_t const currSize = results.GetCount();
  ASSERT_LESS_OR_EQUAL(prevSize, currSize, ());

  for (size_t i = prevSize; i < currSize; ++i)
    m_productInfo.push_back(m_delegate.GetProductInfo(results[i]));

  m_delegate.RunUITask([onResults = m_onResults, results, productInfo = m_productInfo]() mutable
  { onResults(std::move(results), std::move(productInfo)); });
}
}  // namespace search

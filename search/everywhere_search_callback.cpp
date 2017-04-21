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
  auto const prevSize = m_isLocalAdsCustomer.size();
  ASSERT_LESS_OR_EQUAL(prevSize, results.GetCount(), ());

  for (size_t i = prevSize; i < results.GetCount(); ++i)
    m_isLocalAdsCustomer.push_back(m_delegate.IsLocalAdsCustomer(results[i]));

  ASSERT_EQUAL(m_isLocalAdsCustomer.size(), results.GetCount(), ());
  m_onResults(results, m_isLocalAdsCustomer);
}
}  // namespace search

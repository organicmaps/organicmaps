#include "map/discovery/discovery_search_callback.hpp"

#include <cstddef>
#include <utility>

namespace search
{
DiscoverySearchCallback::DiscoverySearchCallback(ProductInfo::Delegate & delegate,
                                                 DiscoverySearchParams::OnResults onResults)
  : m_delegate(delegate)
  , m_onResults(std::move(onResults))
{
}

void DiscoverySearchCallback::operator()(Results const & results)
{
  auto const prevSize = m_productInfo.size();
  ASSERT_LESS_OR_EQUAL(prevSize, results.GetCount(), ());

  for (size_t i = prevSize; i < results.GetCount(); ++i)
  {
    m_productInfo.push_back(m_delegate.GetProductInfo(results[i]));
  }

  ASSERT_EQUAL(m_productInfo.size(), results.GetCount(), ());
  m_onResults(results, m_productInfo);
}
}  // namespace search

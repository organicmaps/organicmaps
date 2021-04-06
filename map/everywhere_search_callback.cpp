#include "map/everywhere_search_callback.hpp"

#include <cstddef>
#include <utility>

namespace search
{
EverywhereSearchCallback::EverywhereSearchCallback(
    ProductInfo::Delegate & productInfoDelegate,
    EverywhereSearchParams::OnResults onResults)
  : m_productInfoDelegate(productInfoDelegate)
  , m_onResults(std::move(onResults))
{
}

void EverywhereSearchCallback::operator()(Results const & results)
{
  auto const prevSize = m_productInfo.size();
  ASSERT_LESS_OR_EQUAL(prevSize, results.GetCount(), ());

  LOG(LINFO, ("Emitted", results.GetCount() - prevSize, "search results."));

  for (size_t i = prevSize; i < results.GetCount(); ++i)
  {
    m_productInfo.push_back(m_productInfoDelegate.GetProductInfo(results[i]));
  }

  ASSERT_EQUAL(m_productInfo.size(), results.GetCount(), ());
  m_onResults(results, m_productInfo);
}
}  // namespace search

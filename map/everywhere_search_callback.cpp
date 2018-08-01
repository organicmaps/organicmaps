#include "map/everywhere_search_callback.hpp"

#include <cstddef>
#include <utility>

namespace search
{
EverywhereSearchCallback::EverywhereSearchCallback(
    Delegate & hotelsDelegate, ProductInfo::Delegate & productInfoDelegate,
    booking::filter::Tasks const & bookingFilterTasks, EverywhereSearchParams::OnResults onResults)
  : m_hotelsDelegate(hotelsDelegate)
  , m_productInfoDelegate(productInfoDelegate)
  , m_onResults(std::move(onResults))
  , m_bookingFilterTasks(bookingFilterTasks)
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

  if (results.IsEndedNormal() && results.GetType() == Results::Type::Hotels &&
      !m_bookingFilterTasks.IsEmpty())
  {
    m_hotelsDelegate.FilterResultsForHotelsQuery(m_bookingFilterTasks, results,
                                                 false /* inViewport */);
  }

  ASSERT_EQUAL(m_productInfo.size(), results.GetCount(), ());
  m_onResults(results, m_productInfo);
}
}  // namespace search

#include "everywhere_search_callback.hpp"

#include <cstddef>
#include <utility>

namespace search
{
EverywhereSearchCallback::EverywhereSearchCallback(Delegate & delegate,
                                                   booking::filter::Tasks const & bookingFilterTasks,
                                                   EverywhereSearchParams::OnResults onResults)
  : m_delegate(delegate)
  , m_onResults(std::move(onResults))
  , m_bookingFilterTasks(bookingFilterTasks)
{
}

void EverywhereSearchCallback::operator()(Results const & results)
{
  auto const prevSize = m_productInfo.size();
  ASSERT_LESS_OR_EQUAL(prevSize, results.GetCount(), ());

  for (size_t i = prevSize; i < results.GetCount(); ++i)
  {
    m_productInfo.push_back(m_delegate.GetProductInfo(results[i]));
  }

  if (results.IsEndedNormal() && results.GetType() == Results::Type::Hotels)
  {
    m_delegate.FilterResultsForHotelsQuery(m_bookingFilterTasks, results, false /* inViewport */);
  }

  ASSERT_EQUAL(m_productInfo.size(), results.GetCount(), ());
  m_onResults(results, m_productInfo);
}
}  // namespace search

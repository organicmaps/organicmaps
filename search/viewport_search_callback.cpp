#include "search/viewport_search_callback.hpp"

#include "search/result.hpp"

#include "base/assert.hpp"

namespace search
{
ViewportSearchCallback::ViewportSearchCallback(Delegate & delegate, OnResults onResults)
  : m_delegate(delegate)
  , m_onResults(move(onResults))
  , m_hotelsModeSet(false)
  , m_firstCall(true)
  , m_lastResultsSize(0)
{
}

void ViewportSearchCallback::operator()(Results const & results)
{
  ASSERT_LESS_OR_EQUAL(m_lastResultsSize, results.GetCount(), ());
  m_hotelsClassif.Add(results.begin() + m_lastResultsSize, results.end());
  m_lastResultsSize = results.GetCount();

  if (!m_hotelsModeSet && m_hotelsClassif.IsHotelResults())
  {
    m_delegate.SetHotelDisplacementMode();
    m_hotelsModeSet = true;
  }

  if (!results.IsEndMarker())
  {
    auto & delegate = m_delegate;
    bool const firstCall = m_firstCall;

    m_delegate.RunUITask([&delegate, firstCall, results]() {
      if (!delegate.IsViewportSearchActive())
        return;

      if (firstCall)
        delegate.ClearViewportSearchResults();

      delegate.ShowViewportSearchResults(results);
    });
  }

  if (m_onResults)
    m_onResults(results);

  m_firstCall = false;
}
}  // namespace search

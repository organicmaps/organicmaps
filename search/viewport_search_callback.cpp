#include "search/viewport_search_callback.hpp"

#include "search/result.hpp"

namespace search
{
ViewportSearchCallback::ViewportSearchCallback(Delegate & delegate, TOnResults onResults)
  : m_delegate(delegate), m_onResults(move(onResults)), m_hotelsModeSet(false), m_firstCall(true)
{
}

void ViewportSearchCallback::operator()(Results const & results)
{
  m_hotelsClassif.AddBatch(results);

  if (!m_hotelsModeSet && m_hotelsClassif.IsHotelQuery())
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

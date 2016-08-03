#include "search/interactive_search_callback.hpp"

#include "search/result.hpp"

namespace search
{
InteractiveSearchCallback::InteractiveSearchCallback(TSetDisplacementMode && setMode,
                                                     TOnResults && onResults)
  : m_setMode(move(setMode)), m_onResults(move(onResults)), m_hotelsModeSet(false)
{
}

void InteractiveSearchCallback::operator()(search::Results const & results)
{
  m_hotelsClassif.AddBatch(results);

  if (!m_hotelsModeSet && m_hotelsClassif.IsHotelQuery())
  {
    m_setMode();
    m_hotelsModeSet = true;
  }

  m_onResults(results);
}
}  // namespace search

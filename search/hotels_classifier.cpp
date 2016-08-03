#include "search/hotels_classifier.hpp"

#include "search/result.hpp"

namespace search
{
void HotelsClassifier::AddBatch(Results const & results)
{
  if (results.IsEndMarker())
    return;

  for (auto const & result : results)
  {
    ++m_numResults;
    if (result.m_metadata.m_isHotel)
      ++m_numHotels;
  }
}

bool HotelsClassifier::IsHotelQuery() const
{
  // Threshold used to activate hotels mode. Probably is too strict,
  // but we don't have statistics now.
  double const kThreshold = 0.95;

  return m_numResults == 0 ? false : m_numHotels >= kThreshold * m_numResults;
}
}  // namespace search

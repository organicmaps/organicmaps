#include "search/hotels_classifier.hpp"

#include "std/cstdint.hpp"

namespace search
{
// static
bool HotelsClassifier::IsHotelResults(Results const & results)
{
  HotelsClassifier classifier;
  classifier.Add(results.begin(), results.end());
  return classifier.IsHotelResults();
}

void HotelsClassifier::Add(Results::ConstIter begin, Results::ConstIter end)
{
  for (; begin != end; ++begin)
  {
    m_numHotels += (*begin).m_metadata.m_isHotel;
    ++m_numResults;
  }
}

bool HotelsClassifier::IsHotelResults() const
{
  // Threshold used to activate hotels mode. Probably is too strict,
  // but we don't have statistics now.
  double const kThreshold = 0.75;

  return m_numResults == 0 ? false : m_numHotels >= kThreshold * m_numResults;
}
}  // namespace search

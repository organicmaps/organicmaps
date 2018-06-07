#include "search/hotels_classifier.hpp"

#include "search/result.hpp"

#include "std/cstdint.hpp"

namespace search
{
// static
bool HotelsClassifier::IsHotelResults(Results const & results)
{
  HotelsClassifier classifier;
  for (auto const & r : results)
    classifier.Add(r);

  return classifier.IsHotelResults();
}

void HotelsClassifier::Add(Result const & result)
{
  if (result.m_metadata.m_isHotel)
   ++m_numHotels;

  ++m_numResults;
}

void HotelsClassifier::Clear()
{
  m_numHotels = 0;
  m_numResults = 0;
}

bool HotelsClassifier::IsHotelResults() const
{
  // Threshold used to activate hotels mode. Probably is too strict,
  // but we don't have statistics now.
  double const kThreshold = 0.75;

  return m_numResults == 0 ? false : m_numHotels >= kThreshold * m_numResults;
}
}  // namespace search

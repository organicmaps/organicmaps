#include "search/hotels_classifier.hpp"

#include "search/result.hpp"

#include "indexer/ftypes_matcher.hpp"

#include "base/logging.hpp"

using namespace std;

namespace search
{
void HotelsClassifier::Add(Result const & result)
{
  if (result.m_details.m_isHotel)
    ++m_numHotels;

  ++m_numResults;
}

void HotelsClassifier::PrecheckHotelQuery(vector<uint32_t> const & types)
{
  m_looksLikeHotelQuery = ftypes::IsHotelChecker::Instance()(types);
}

void HotelsClassifier::Clear()
{
  m_numHotels = 0;
  m_numResults = 0;
  m_looksLikeHotelQuery = false;
}

bool HotelsClassifier::IsHotelResults() const
{
  if (m_looksLikeHotelQuery)
    return true;

  // Threshold used to activate hotels mode. Probably is too strict,
  // but we don't have statistics now.
  double const kThreshold = 0.75;

  return m_numResults == 0 ? false : m_numHotels >= kThreshold * m_numResults;
}
}  // namespace search

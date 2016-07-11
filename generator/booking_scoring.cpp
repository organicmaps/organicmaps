#include "generator/booking_scoring.hpp"

#include "generator/booking_dataset.hpp"

#include "indexer/search_string_utils.hpp"
#include "indexer/search_delimiters.hpp"

#include "geometry/distance_on_sphere.hpp"

#include "base/collection_cast.hpp"

namespace generator
{
namespace booking_scoring
{
namespace
{
// Calculated with tools/python/booking_hotels_quality.py.
double constexpr kOptimalThreshold = 0.151001;

template <typename T, typename U>
struct decay_equiv :
    std::is_same<typename std::decay<T>::type, U>::type
{};

set<strings::UniString> StringToSetOfWords(string const & str)
{
  vector<strings::UniString> result;
  search::NormalizeAndTokenizeString(str, result, search::Delimiters{});
  return my::collection_cast<set>(result);
}

// TODO(mgsergio): Update existing one in base or wherever...
// Or just use one from boost.
struct CounterIterator
{
  template<typename T, typename = typename enable_if<!decay_equiv<T, CounterIterator>::value>::type>
  CounterIterator & operator=(T const &) { ++m_count; return *this; }
  CounterIterator & operator++() { return *this; }
  CounterIterator & operator++(int) { return *this; }
  CounterIterator & operator*() { return *this; }
  uint32_t Count() const { return m_count; }

  uint32_t m_count = 0;
};

double StringSimilarityScore(string const & a, string const & b)
{
  auto const aWords = StringToSetOfWords(a);
  auto const bWords = StringToSetOfWords(b);

  auto const intersectionCard = set_intersection(begin(aWords), end(aWords),
                                                 begin(bWords), end(bWords),
                                                 CounterIterator()).Count();
  auto const aLikeBScore = static_cast<double>(intersectionCard) / aWords.size();
  auto const bLikeAScore = static_cast<double>(intersectionCard) / bWords.size();

  return aLikeBScore * bLikeAScore;
}

double GetLinearNormDistanceScrore(double distance)
{
  distance = my::clamp(distance, 0, BookingDataset::kDistanceLimitInMeters);
  return 1.0 - distance / BookingDataset::kDistanceLimitInMeters;
}

double GetNameSimilarityScore(string const & booking_name, string const & osm_name)
{
  return StringSimilarityScore(booking_name, osm_name);
}
}  // namespace

double BookingMatchScore::GetMatchingScore() const
{
  return m_linearNormDistanceScore * m_nameSimilarityScore;
}

bool BookingMatchScore::IsMatched() const
{
  return GetMatchingScore() > kOptimalThreshold;
}

BookingMatchScore Match(BookingDataset::Hotel const & h, OsmElement const & e)
{
  BookingMatchScore score;

  auto const distance = ms::DistanceOnEarth(e.lat, e.lon, h.lat, h.lon);
  score.m_linearNormDistanceScore = GetLinearNormDistanceScrore(distance);

  string osmHotelName;
  score.m_nameSimilarityScore = e.GetTag("name", osmHotelName)
      ? GetNameSimilarityScore(h.name, osmHotelName) : 0;

  return score;
}
}  // namespace booking_scoring
}  // namespace generator

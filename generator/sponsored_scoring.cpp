#include "generator/sponsored_scoring.hpp"

#include "search/ranking_utils.hpp"

#include "indexer/search_string_utils.hpp"

#include "base/math.hpp"

#include <algorithm>
#include <vector>


namespace generator
{
namespace sponsored
{
namespace
{
using StringT = strings::UniString;
class SkipTokens
{
  std::set<StringT> m_skip;
public:
  SkipTokens()
  {
    /// @todo Add other common terms?
    m_skip.insert(strings::MakeUniString("hotel"));
  }
  bool Has(StringT const & s) const
  {
    return m_skip.count(s) > 0;
  }
};

using WeightedBagOfWords = std::vector<std::pair<StringT, double>>;

std::vector<StringT> StringToWords(std::string const & str)
{
  auto result = search::NormalizeAndTokenizeString(str);

  static SkipTokens toSkip;
  auto it = std::remove_if(result.begin(), result.end(), [](StringT const & s)
  {
    return toSkip.Has(s) || search::IsStopWord(s);
  });

  // In case if name is like "The Hotel".
  if (std::distance(result.begin(), it) > 0)
    result.erase(it, result.end());

  std::sort(result.begin(), result.end());
  return result;
}

WeightedBagOfWords MakeWeightedBagOfWords(std::vector<StringT> const & words)
{
  // TODO(mgsergio): Calculate tf-idsf score for every word.
  auto constexpr kTfIdfScorePlaceholder = 1;

  WeightedBagOfWords result;
  for (size_t i = 0; i < words.size(); ++i)
  {
    result.emplace_back(words[i], kTfIdfScorePlaceholder);
    while (i + 1 < words.size() && words[i] == words[i + 1])
    {
      result.back().second += kTfIdfScorePlaceholder;  // TODO(mgsergio): tf-idf score for result[i].frist;
      ++i;
    }
  }
  return result;
}

double WeightedBagsDotProduct(WeightedBagOfWords const & lhs, WeightedBagOfWords const & rhs)
{
  double result = 0;

  auto lhsIt = begin(lhs);
  auto rhsIt = begin(rhs);

  while (lhsIt != end(lhs) && rhsIt != end(rhs))
  {
    if (lhsIt->first == rhsIt->first)
    {
      result += lhsIt->second * rhsIt->second;
      ++lhsIt;
      ++rhsIt;
    }
    else if (lhsIt->first < rhsIt->first)
    {
      ++lhsIt;
    }
    else
    {
      ++rhsIt;
    }
  }

  return result;
}

double WeightedBagOfWordsCos(WeightedBagOfWords const & lhs, WeightedBagOfWords const & rhs)
{
  auto const product = WeightedBagsDotProduct(lhs, rhs);
  auto const lhsLength = sqrt(WeightedBagsDotProduct(lhs, lhs));
  auto const rhsLength = sqrt(WeightedBagsDotProduct(rhs, rhs));

  // WeightedBagsDotProduct returns 0.0 if lhs.empty() || rhs.empty() or
  // if every element of either lhs or rhs is 0.0.
  if (product == 0.0)
    return 0.0;

  return product / (lhsLength * rhsLength);
}

double GetLinearNormDistanceScore(double distance, double const maxDistance)
{
  CHECK_NOT_EQUAL(maxDistance, 0.0, ("maxDistance cannot be 0."));
  distance = base::Clamp(distance, 0.0, maxDistance);
  return 1.0 - distance / maxDistance;
}

double GetNameSimilarityScore(std::string const & booking_name, std::string const & osm_name)
{
  auto const aws = MakeWeightedBagOfWords(StringToWords(booking_name));
  auto const bws = MakeWeightedBagOfWords(StringToWords(osm_name));

  if (aws.empty() && bws.empty())
    return 1.0;
  if (aws.empty() || bws.empty())
    return 0.0;

  return WeightedBagOfWordsCos(aws, bws);
}
}  // namespace

MatchStats::MatchStats(double distM, double distLimitM, std::string const & name, std::string const & fbName)
  : m_distance(distM)
{
  m_linearNormDistanceScore = GetLinearNormDistanceScore(distM, distLimitM);

  // TODO(mgsergio): Check all translations and use the best one.
  m_nameSimilarityScore = GetNameSimilarityScore(name, fbName);
}

} // namespace sponsored
} // namespace generator

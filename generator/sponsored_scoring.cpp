#include "generator/sponsored_scoring.hpp"

#include "indexer/search_delimiters.hpp"
#include "indexer/search_string_utils.hpp"

#include "geometry/distance_on_sphere.hpp"

// #include "base/stl_iterator.hpp"

#include "std/algorithm.hpp"
#include "std/vector.hpp"

namespace
{
using WeightedBagOfWords = vector<pair<strings::UniString, double>>;

vector<strings::UniString> StringToSetOfWords(string const & str)
{
  vector<strings::UniString> result;
  search::NormalizeAndTokenizeString(str, result, search::Delimiters{});
  sort(begin(result), end(result));
  return result;
}

WeightedBagOfWords MakeWeightedBagOfWords(vector<strings::UniString> const & words)
{
  // TODO(mgsergio): Calculate tf-idsf score for every word.
  auto constexpr kTfIdfScorePlaceholder = 1;

  WeightedBagOfWords result;
  for (auto i = 0; i < words.size(); ++i)
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
  double result{};

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

  if (product == 0.0)
    return 0.0;

  return product / (lhsLength * rhsLength);
}
}  // namespace

namespace generator
{
namespace impl
{
double GetLinearNormDistanceScore(double distance, double const maxDistance)
{
  distance = my::clamp(distance, 0, maxDistance);
  return 1.0 - distance / maxDistance;
}

double GetNameSimilarityScore(string const & booking_name, string const & osm_name)
{
  auto const aws = MakeWeightedBagOfWords(StringToSetOfWords(booking_name));
  auto const bws = MakeWeightedBagOfWords(StringToSetOfWords(osm_name));

  if (aws.empty() && bws.empty())
    return 1.0;
  if (aws.empty() || bws.empty())
    return 0.0;

  return WeightedBagOfWordsCos(aws, bws);
}
}  // namespace impl
}  // namespace generator

#include "search/address_estimator.hpp"

#include "indexer/search_string_utils.hpp"

#include "geometry/distance_on_sphere.hpp"
#include "geometry/mercator.hpp"

#include "base/string_utils.hpp"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <limits>
#include <optional>
#include <set>
#include <string_view>
#include <vector>

namespace search
{
namespace
{
uint64_t constexpr kMaxNearbyHouseNumberDifference = 24;
uint64_t constexpr kMaxSupportNumberGap = 24;
double constexpr kMaxSupportDistanceMeters = 100.0;
double constexpr kMaxExtrapolationDistanceMeters = 50.0;

struct ParsedAddress
{
  uint64_t m_houseNumber = 0;
  std::string m_street;
  std::vector<std::string> m_streetTokens;
};

struct Candidate
{
  size_t m_index = 0;
  uint64_t m_houseNumber = 0;
  m2::PointD m_center;
  std::string m_street;
  std::string m_streetKey;
  Result const * m_result = nullptr;
};

struct Estimate
{
  Candidate const * m_first = nullptr;
  Candidate const * m_second = nullptr;
  m2::PointD m_center;
  uint64_t m_score = std::numeric_limits<uint64_t>::max();
};

std::string CanonicalizeToken(std::string token)
{
  if (token.size() > 2)
  {
    auto const suffix = token.substr(token.size() - 2);
    auto const number = token.substr(0, token.size() - 2);
    if ((suffix == "st" || suffix == "nd" || suffix == "rd" || suffix == "th") &&
        std::all_of(number.begin(), number.end(), [](char c) { return c >= '0' && c <= '9'; }))
      return number;
  }

  return strings::ToUtf8(GetNormalizedStreetName(token));
}

std::vector<std::string> Tokenize(std::string_view value)
{
  std::vector<std::string> tokens;
  for (auto const & token : NormalizeAndTokenizeString(value))
    tokens.push_back(CanonicalizeToken(strings::ToUtf8(token)));
  return tokens;
}

std::optional<ParsedAddress> ParseAddress(std::string_view value)
{
  auto const tokens = Tokenize(value);
  if (tokens.size() < 2)
    return {};

  uint64_t houseNumber = 0;
  if (!strings::to_uint(tokens.front(), houseNumber))
    return {};

  size_t firstNonSpace = value.find_first_not_of(" \t\r\n");
  if (firstNonSpace == std::string_view::npos)
    return {};
  size_t const numberEnd = value.find_first_not_of("0123456789", firstNonSpace);
  if (numberEnd == std::string_view::npos || numberEnd == firstNonSpace)
    return {};
  size_t const streetStart = value.find_first_not_of(" ,\t\r\n", numberEnd);
  if (streetStart == std::string_view::npos)
    return {};

  ParsedAddress address;
  address.m_houseNumber = houseNumber;
  address.m_street = std::string(value.substr(streetStart));
  address.m_streetTokens = Tokenize(address.m_street);
  return address;
}

bool StreetMatchesQuery(std::vector<std::string> const & queryTokens, std::vector<std::string> const & streetTokens,
                        std::vector<std::string> const & addressTokens)
{
  if (queryTokens.size() < streetTokens.size())
    return false;
  std::multiset<std::string> remaining(queryTokens.begin(), queryTokens.end());
  for (auto const & token : streetTokens)
  {
    auto const it = remaining.find(token);
    if (it == remaining.end())
      return false;
    remaining.erase(it);
  }
  if (streetTokens.empty())
    return false;

  std::multiset<std::string> address(addressTokens.begin(), addressTokens.end());
  for (auto const & token : remaining)
  {
    auto const it = address.find(token);
    if (it == address.end())
      return false;
    address.erase(it);
  }
  return true;
}

std::optional<Candidate> ParseCandidate(size_t index, Result const & result,
                                        std::vector<std::string> const & queryTokens, uint64_t requested)
{
  if (!result.HasPoint() || result.IsSuggest())
    return {};

  auto const parsed = ParseAddress(result.GetString());
  if (!parsed || parsed->m_houseNumber % 2 != requested % 2 ||
      !StreetMatchesQuery(queryTokens, parsed->m_streetTokens, Tokenize(result.GetAddress())))
    return {};

  uint64_t const difference = parsed->m_houseNumber > requested ? parsed->m_houseNumber - requested
                                                                 : requested - parsed->m_houseNumber;
  if (difference > kMaxNearbyHouseNumberDifference)
    return {};

  return Candidate{index, parsed->m_houseNumber, result.GetFeatureCenter(), parsed->m_street,
                   strings::ToUtf8(GetNormalizedStreetName(parsed->m_street)), &result};
}

double DistanceMeters(m2::PointD const & first, m2::PointD const & second)
{
  auto const firstLatLon = mercator::ToLatLon(first);
  auto const secondLatLon = mercator::ToLatLon(second);
  return ms::DistanceOnEarth(firstLatLon, secondLatLon);
}

std::optional<Estimate> FindEstimate(std::vector<Candidate> const & candidates, uint64_t requested)
{
  std::optional<Estimate> best;
  for (size_t i = 0; i < candidates.size(); ++i)
  {
    for (size_t j = i + 1; j < candidates.size(); ++j)
    {
      auto const * first = &candidates[i];
      auto const * second = &candidates[j];
      if (first->m_streetKey != second->m_streetKey || first->m_houseNumber == second->m_houseNumber)
        continue;
      if (first->m_houseNumber > second->m_houseNumber)
        std::swap(first, second);

      uint64_t const gap = second->m_houseNumber - first->m_houseNumber;
      if (gap > kMaxSupportNumberGap ||
          (requested < first->m_houseNumber && first->m_houseNumber - requested > gap) ||
          (requested > second->m_houseNumber && requested - second->m_houseNumber > gap))
        continue;

      double const supportDistance = DistanceMeters(first->m_center, second->m_center);
      if (supportDistance > kMaxSupportDistanceMeters)
        continue;

      double const ratio = (static_cast<double>(requested) - static_cast<double>(first->m_houseNumber)) /
                           static_cast<double>(gap);
      double const extrapolationRatio = ratio < 0.0 ? -ratio : std::max(0.0, ratio - 1.0);
      if (supportDistance * extrapolationRatio > kMaxExtrapolationDistanceMeters)
        continue;

      uint64_t const score = std::max(requested > first->m_houseNumber ? requested - first->m_houseNumber
                                                                       : first->m_houseNumber - requested,
                                      requested > second->m_houseNumber ? requested - second->m_houseNumber
                                                                        : second->m_houseNumber - requested);
      if (best && best->m_score <= score)
        continue;

      best = Estimate{first, second, first->m_center + (second->m_center - first->m_center) * ratio, score};
    }
  }
  return best;
}

std::string MakeEstimatedAddress(std::string const & address, uint64_t oldNumber, uint64_t requested)
{
  auto const oldNumberString = std::to_string(oldNumber);
  size_t const start = address.find_first_not_of(" \t\r\n");
  if (start != std::string::npos && address.compare(start, oldNumberString.size(), oldNumberString) == 0)
  {
    std::string result = address;
    result.replace(start, oldNumberString.size(), std::to_string(requested));
    return result;
  }
  return address;
}
}  // namespace

Results MakeEstimatedAddressResults(std::string const & query, Results const & results)
{
  auto const requestedAddress = ParseAddress(query);
  if (!requestedAddress)
    return results;

  std::vector<Candidate> candidates;
  for (size_t i = 0; i < results.GetCount(); ++i)
  {
    auto candidate = ParseCandidate(i, results[i], requestedAddress->m_streetTokens, requestedAddress->m_houseNumber);
    if (candidate)
      candidates.push_back(std::move(*candidate));
  }
  if (candidates.empty())
    return results;

  auto const exact = std::find_if(candidates.begin(), candidates.end(), [&](Candidate const & candidate)
  { return candidate.m_houseNumber == requestedAddress->m_houseNumber; });
  auto const estimate = exact == candidates.end() ? FindEstimate(candidates, requestedAddress->m_houseNumber)
                                                   : std::optional<Estimate>{};

  std::set<size_t> candidateIndices;
  for (auto const & candidate : candidates)
    candidateIndices.insert(candidate.m_index);
  size_t insertionIndex = 0;
  while (insertionIndex < results.GetCount() && results[insertionIndex].IsSuggest())
    ++insertionIndex;

  Results transformed;
  for (size_t i = 0; i < results.GetCount(); ++i)
  {
    if (i == insertionIndex && estimate)
    {
      uint64_t const firstDifference = estimate->m_first->m_houseNumber > requestedAddress->m_houseNumber
                                           ? estimate->m_first->m_houseNumber - requestedAddress->m_houseNumber
                                           : requestedAddress->m_houseNumber - estimate->m_first->m_houseNumber;
      uint64_t const secondDifference = estimate->m_second->m_houseNumber > requestedAddress->m_houseNumber
                                            ? estimate->m_second->m_houseNumber - requestedAddress->m_houseNumber
                                            : requestedAddress->m_houseNumber - estimate->m_second->m_houseNumber;
      Candidate const & nearest = firstDifference <= secondDifference ? *estimate->m_first : *estimate->m_second;
      Result estimated(estimate->m_center,
                       std::to_string(requestedAddress->m_houseNumber) + ", " + nearest.m_street);
      estimated.SetAddress(MakeEstimatedAddress(nearest.m_result->GetAddress(), nearest.m_houseNumber,
                                                requestedAddress->m_houseNumber));
      estimated.SetType(Result::Type::LatLon);
      estimated.SetEstimatedAddress(true);
      transformed.AddResultNoChecks(std::move(estimated));
    }

    bool const keepCandidate = exact != candidates.end() ? i == exact->m_index : !estimate;
    if (!candidateIndices.contains(i) || keepCandidate)
      transformed.AddResultNoChecks(Result(results[i]));
  }

  if (results.IsEndMarker())
    transformed.SetEndMarker(results.IsEndedCancelled());
  return transformed;
}

bool IsAddressResultMatchingQuery(std::string const & query, Result const & result)
{
  auto const requested = ParseAddress(query);
  if (!requested)
    return false;
  auto const candidate = ParseCandidate(0, result, requested->m_streetTokens, requested->m_houseNumber);
  return candidate && candidate->m_houseNumber == requested->m_houseNumber;
}
}  // namespace search

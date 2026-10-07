#include "search/address_matcher.hpp"
#include "search/string_utils.hpp"

#include "indexer/ftypes_matcher.hpp"
#include "indexer/postcodes_matcher.hpp"
#include "indexer/search_string_utils.hpp"

#include "geometry/mercator.hpp"

#include "base/string_utils.hpp"

#include <algorithm>
#include <cctype>
#include <cstdint>
#include <optional>
#include <set>
#include <string_view>
#include <vector>

namespace search
{
namespace
{
uint64_t constexpr kMaxNearbyHouseNumberDifference = 24;
struct ParsedAddress
{
  uint64_t m_houseNumber = 0;
  std::string m_houseIdentifier;
  std::string m_street;
  std::vector<std::string> m_streetTokens;
};

struct Candidate
{
  size_t m_index = 0;
  uint64_t m_houseNumber = 0;
  std::string m_houseIdentifier;
};

std::string CanonicalizeToken(std::string token)
{
  if (token == "twp" || token == "twnshp")
    return "township";
  if (token == "boro")
    return "borough";
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
  // Search accepts these country aliases; address validation must accept the same metadata.
  for (size_t i = 0; i < tokens.size(); ++i)
  {
    if (tokens[i] == "us")
      tokens[i] = "usa";
    if (tokens[i] == "gb" || tokens[i] == "gbr")
      tokens[i] = "uk";
    if (i + 1 < tokens.size() && tokens[i] == "czech" && tokens[i + 1] == "republic")
    {
      tokens[i] = "czechia";
      tokens.erase(tokens.begin() + i + 1);
    }
    if (i + 1 < tokens.size() &&
        ((tokens[i] == "united" && tokens[i + 1] == "kingdom") || (tokens[i] == "great" && tokens[i + 1] == "britain")))
    {
      tokens[i] = "uk";
      tokens.erase(tokens.begin() + i + 1);
    }
    if (i + 1 < tokens.size() && tokens[i] == "united" && tokens[i + 1] == "states")
    {
      size_t end = i + 2;
      if (end + 1 < tokens.size() && tokens[end] == "of" && tokens[end + 1] == "america")
        end += 2;
      tokens[i] = "usa";
      tokens.erase(tokens.begin() + i + 1, tokens.begin() + end);
    }
  }
  return tokens;
}

std::optional<std::pair<uint64_t, std::string>> ParseHouseIdentifier(std::string_view value)
{
  auto const numberEnd = value.find_first_not_of("0123456789");
  auto const digitCount = numberEnd == std::string_view::npos ? value.size() : numberEnd;
  uint64_t number = 0;
  if (!strings::to_uint(value.substr(0, digitCount), number))
    return {};
  std::string identifier = std::to_string(number);
  size_t end = digitCount;
  if (end < value.size() && std::isalpha(static_cast<unsigned char>(value[end])))
    identifier += static_cast<char>(std::tolower(static_cast<unsigned char>(value[end++])));
  while (end < value.size() && (value[end] == '/' || value[end] == '-'))
  {
    identifier += value[end++];
    size_t const start = end;
    while (end < value.size() && value[end] >= '0' && value[end] <= '9')
      ++end;
    uint64_t second = 0;
    if (!strings::to_uint(value.substr(start, end - start), second))
      return {};
    identifier += std::to_string(second);
    if (end < value.size() && std::isalpha(static_cast<unsigned char>(value[end])))
      identifier += static_cast<char>(std::tolower(static_cast<unsigned char>(value[end++])));
  }
  if (end != value.size())
    return {};
  return std::pair{number, std::move(identifier)};
}

std::optional<ParsedAddress> ParseAddress(std::string_view value)
{
  // Map regions format addresses in either house/street or street/house order.
  auto const comma = value.rfind(',');
  if (comma != std::string_view::npos)
  {
    std::string houseField(value.substr(comma + 1));
    strings::Trim(houseField);
    auto const house = ParseHouseIdentifier(houseField);
    if (house)
    {
      auto const street = value.substr(0, comma);
      auto tokens = Tokenize(street);
      if (!tokens.empty())
        return ParsedAddress{house->first, house->second, std::string(street), std::move(tokens)};
    }
  }
  // House-last formats are common outside North America. A comma bounds the street
  // component so numbers in the locality or postcode cannot become the house number.
  auto const firstComma = value.find(',');
  auto const component = value.substr(0, firstComma);
  auto const last = component.find_last_not_of(" \t\r\n");
  if (last != std::string_view::npos &&
      value.find_first_not_of(" \t\r\n0123456789") == value.find_first_not_of(" \t\r\n"))
  {
    auto const separator = component.find_last_of(" \t\r\n", last);
    if (separator != std::string_view::npos)
    {
      auto const house = ParseHouseIdentifier(component.substr(separator + 1, last - separator));
      auto const street = component.substr(0, separator);
      auto tokens = Tokenize(street);
      if (house && !tokens.empty())
      {
        if (firstComma != std::string_view::npos)
        {
          auto const context = Tokenize(value.substr(firstComma + 1));
          tokens.insert(tokens.end(), context.begin(), context.end());
        }
        return ParsedAddress{house->first, house->second, std::string(street), std::move(tokens)};
      }
    }
  }
  auto const tokens = Tokenize(value);
  if (tokens.size() < 2)
    return {};

  size_t firstNonSpace = value.find_first_not_of(" \t\r\n");
  if (firstNonSpace == std::string_view::npos)
    return {};
  size_t const numberEnd = value.find_first_not_of("0123456789", firstNonSpace);
  if (numberEnd == std::string_view::npos || numberEnd == firstNonSpace)
    return {};
  size_t const identifierEnd = value.find_first_of(" ,\t\r\n", firstNonSpace);
  if (identifierEnd == std::string_view::npos)
    return {};
  size_t const streetStart = value.find_first_not_of(" ,\t\r\n", identifierEnd);
  if (streetStart == std::string_view::npos)
    return {};
  auto const house = ParseHouseIdentifier(value.substr(firstNonSpace, identifierEnd - firstNonSpace));
  if (!house)
    return {};

  ParsedAddress address;
  address.m_houseNumber = house->first;
  address.m_houseIdentifier = house->second;
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

std::optional<Candidate> ParseCandidate(size_t index, Result const & result, AddressQuery const & requested,
                                        std::string const & expectedStreet = {})
{
  if (!result.HasPoint() || result.IsSuggest())
    return {};

  auto const parsed = ParseAddress(result.GetString());
  if (!parsed)
    return {};

  if (!expectedStreet.empty())
  {
    auto const expected = ParseAddress(RemoveAddressDetails(expectedStreet));
    if (!expected || std::multiset<std::string>(expected->m_streetTokens.begin(), expected->m_streetTokens.end()) !=
                         std::multiset<std::string>(parsed->m_streetTokens.begin(), parsed->m_streetTokens.end()))
      return {};
  }

  if (!StreetMatchesQuery(requested.m_streetAndLocalityTokens, parsed->m_streetTokens, Tokenize(result.GetAddress())))
    return {};

  uint64_t const difference = parsed->m_houseNumber > requested.m_houseNumber
                                ? parsed->m_houseNumber - requested.m_houseNumber
                                : requested.m_houseNumber - parsed->m_houseNumber;
  bool const exact = parsed->m_houseIdentifier == requested.m_houseIdentifier;
  if (!exact && (difference > kMaxNearbyHouseNumberDifference ||
                 parsed->m_houseIdentifier != std::to_string(parsed->m_houseNumber) ||
                 requested.m_houseIdentifier != std::to_string(requested.m_houseNumber)))
    return {};

  return Candidate{index, parsed->m_houseNumber, parsed->m_houseIdentifier};
}

}  // namespace

std::optional<AddressQuery> ParseAddressQuery(std::string const & query)
{
  if (query.size() > 4096)
    return {};
  auto parsed = ParseAddress(RemoveAddressDetails(query));
  if (!parsed)
    return {};
  // Keep postal-only queries intact. In a comma-delimited full address, only the
  // context is eligible: numbered streets and compound house identifiers stay untouched.
  auto const comma = query.find(',');
  auto const streetComponent =
      comma == std::string::npos ? std::nullopt : ParseAddress(RemoveAddressDetails(query.substr(0, comma)));
  if (streetComponent && parsed->m_streetTokens.size() >= streetComponent->m_streetTokens.size())
  {
    auto const streetSize = streetComponent->m_streetTokens.size();
    std::multiset<std::string> postcodes;
    auto const hasDigit = [](std::string const & token)
    { return std::any_of(token.begin(), token.end(), [](char c) { return c >= '0' && c <= '9'; }); };
    auto const hasLetter = [](std::string const & token)
    { return std::any_of(token.begin(), token.end(), [](unsigned char c) { return std::isalpha(c); }); };
    // Numeric locality identifiers (e.g. Sector 123) are not automatically postcodes.
    // Recognize a separate postcode field, postcode-before-city, or an alphanumeric tail.
    for (size_t start = comma + 1; start < query.size();)
    {
      auto const end = query.find(',', start);
      auto const field = Tokenize(query.substr(start, end == std::string::npos ? end : end - start));
      if (!field.empty())
      {
        bool const pair = field.size() >= 2 && hasDigit(field[0]) &&
                          (hasDigit(field[1]) || (field[0].size() == 4 && field[1].size() == 2));
        size_t const count = pair && LooksLikePostcode(field[0] + " " + field[1], false) ? 2
                           : hasDigit(field[0]) && LooksLikePostcode(field[0], false)    ? 1
                                                                                         : 0;
        postcodes.insert(field.begin(), field.begin() + count);
        if (count == 0 && field.size() >= 2)
        {
          auto const & last = field.back();
          auto const & previous = field[field.size() - 2];
          if (hasDigit(previous) && hasLetter(previous) && hasDigit(last) && hasLetter(last) &&
              LooksLikePostcode(previous + " " + last, false))
          {
            postcodes.insert(previous);
            postcodes.insert(last);
          }
          else if (hasDigit(last) && hasLetter(last) && LooksLikePostcode(last, false))
            postcodes.insert(last);
          else if (hasLetter(previous) && !hasDigit(previous) && hasDigit(last) && !hasLetter(last) &&
                   LooksLikePostcode(previous + " " + last, false))
            postcodes.insert(last);  // Keep the state/province code that precedes its postcode.
        }
      }
      if (end == std::string::npos)
        break;
      start = end + 1;
    }
    auto & tokens = parsed->m_streetTokens;
    if (tokens.size() > streetSize + postcodes.size())
    {
      for (size_t i = streetSize; i < tokens.size();)
        if (auto const it = postcodes.find(tokens[i]); it != postcodes.end())
        {
          postcodes.erase(it);
          tokens.erase(tokens.begin() + i);
        }
        else
          ++i;
    }
  }
  return AddressQuery{parsed->m_houseNumber, std::move(parsed->m_houseIdentifier), std::move(parsed->m_streetTokens)};
}

std::string DebugPrint(AddressResultMatch match)
{
  switch (match)
  {
  case AddressResultMatch::None: return "None";
  case AddressResultMatch::Nearby: return "Nearby";
  case AddressResultMatch::Interpolated: return "Interpolated";
  case AddressResultMatch::Exact: return "Exact";
  }
  UNREACHABLE();
}

bool IsAddressQuery(std::string const & query)
{
  if (query.size() > 4096)
    return false;
  auto const address = ParseAddressQuery(query);
  if (!address || address->m_streetAndLocalityTokens.empty())
    return false;
  auto const first = query.find_first_not_of(" \t\r\n");
  bool const houseFirst = first != std::string::npos && query[first] >= '0' && query[first] <= '9';
  auto const & firstToken = address->m_streetAndLocalityTokens.front();
  bool const numberedStreet = houseFirst &&
                              (firstToken == "st" || firstToken == "street" || firstToken == "ave" ||
                               firstToken == "avenue" || firstToken == "rd" || firstToken == "road") &&
                              !address->m_houseIdentifier.empty() && address->m_houseIdentifier.back() >= 'a' &&
                              address->m_houseIdentifier.back() <= 'z';
  return std::any_of(address->m_streetAndLocalityTokens.begin(), address->m_streetAndLocalityTokens.end(),
                     [&](std::string const & token)
  {
    auto const normalized = strings::MakeUniString(token);
    if (address->m_streetAndLocalityTokens.size() > 1 && IsStreetSynonym(normalized) &&
        (!numberedStreet || token != address->m_streetAndLocalityTokens.front()))
      return true;
    // Some languages join the street type to its name (e.g. Hauptstraße).
    // Reuse the multilingual dictionary, requiring both a name and a substantial suffix.
    for (size_t i = 2; i + 4 <= normalized.size(); ++i)
      if (IsStreetSynonym(strings::UniString(normalized.begin() + i, normalized.end())))
        return true;
    return false;
  });
}

Results RankAddressResults(std::string const & query, Results const & results, std::string const & expectedStreet)
{
  auto const requestedAddress = ParseAddressQuery(query);
  if (!requestedAddress)
    return results;

  std::vector<Candidate> candidates;
  for (size_t i = 0; i < results.GetCount(); ++i)
  {
    auto candidate = ParseCandidate(i, results[i], *requestedAddress, expectedStreet);
    if (candidate)
      candidates.push_back(std::move(*candidate));
  }
  if (candidates.empty())
    return results;

  std::stable_sort(candidates.begin(), candidates.end(), [&](Candidate const & first, Candidate const & second)
  {
    auto const difference = [&](uint64_t number)
    {
      return number > requestedAddress->m_houseNumber ? number - requestedAddress->m_houseNumber
                                                      : requestedAddress->m_houseNumber - number;
    };
    return difference(first.m_houseNumber) < difference(second.m_houseNumber);
  });

  auto const exact = std::find_if(candidates.begin(), candidates.end(), [&](Candidate const & candidate)
  { return candidate.m_houseIdentifier == requestedAddress->m_houseIdentifier; });
  std::set<size_t> candidateIndices;
  for (auto const & candidate : candidates)
    candidateIndices.insert(candidate.m_index);
  size_t insertionIndex = 0;
  while (insertionIndex < results.GetCount() && results[insertionIndex].IsSuggest())
    ++insertionIndex;

  Results transformed;
  for (size_t i = 0; i < results.GetCount(); ++i)
  {
    if (i == insertionIndex)
    {
      // An address query should not bury its matching addresses below named POIs.
      // Nearby numbers are ordered by difference; ties and other results retain the engine's order.
      for (auto const & candidate : candidates)
        if (exact == candidates.end() || candidate.m_houseIdentifier == requestedAddress->m_houseIdentifier)
          transformed.AddResultNoChecks(Result(results[candidate.m_index]));
    }
    if (!candidateIndices.contains(i))
      transformed.AddResultNoChecks(Result(results[i]));
  }

  if (results.IsEndMarker())
    transformed.SetEndMarker(results.IsEndedCancelled());
  return transformed;
}

AddressResultMatch GetAddressResultMatch(std::string const & query, Result const & result,
                                         std::string const & expectedStreet)
{
  auto const requested = ParseAddressQuery(query);
  if (!requested)
    return AddressResultMatch::None;
  return GetAddressResultMatch(*requested, result, expectedStreet);
}

AddressResultMatch GetAddressResultMatch(AddressQuery const & query, Result const & result,
                                         std::string const & expectedStreet)
{
  auto const candidate = ParseCandidate(0, result, query, expectedStreet);
  if (!candidate)
    return AddressResultMatch::None;
  if (candidate->m_houseIdentifier != query.m_houseIdentifier)
    return AddressResultMatch::Nearby;
  if (result.GetResultType() == Result::Type::Feature &&
      ftypes::IsAddressInterpolChecker::Instance()(result.GetFeatureType()))
    return AddressResultMatch::Interpolated;
  return AddressResultMatch::Exact;
}

bool IsAddressResultMatchingQuery(std::string const & query, Result const & result, std::string const & expectedStreet)
{
  auto const match = GetAddressResultMatch(query, result, expectedStreet);
  return match == AddressResultMatch::Exact || match == AddressResultMatch::Interpolated;
}

bool IsEarlyAddressResultMatchingQuery(std::string const & query, Result const & result,
                                       std::string const & expectedStreet)
{
  if (result.GetResultType() != Result::Type::Feature ||
      !ftypes::IsAddressChecker::Instance()(result.GetFeatureType()) ||
      ftypes::IsAddressInterpolChecker::Instance()(result.GetFeatureType()))
    return false;
  auto const requested = ParseAddressQuery(query);
  auto const street = ParseAddress(RemoveAddressDetails(expectedStreet));
  if (!requested || !street || requested->m_streetAndLocalityTokens.size() <= street->m_streetTokens.size())
    return false;
  return IsAddressResultMatchingQuery(query, result, expectedStreet);
}

Result const * FindUniqueAddressResult(std::string const & query, Results const & results,
                                       std::string const & expectedStreet, bool early)
{
  auto const parsed = ParseAddressQuery(query);
  if (!parsed)
    return nullptr;
  Result const * match = nullptr;
  auto bestKind = AddressResultMatch::None;
  bool ambiguous = false;
  for (auto const & result : results)
  {
    auto const kind = GetAddressResultMatch(*parsed, result, expectedStreet);
    if ((kind != AddressResultMatch::Exact && kind != AddressResultMatch::Interpolated) ||
        (early && !IsEarlyAddressResultMatchingQuery(query, result, expectedStreet)))
      continue;
    if (kind > bestKind)
    {
      bestKind = kind;
      match = &result;
      ambiguous = false;
    }
    else if (kind == bestKind && match->GetFeatureCenter() != result.GetFeatureCenter())
      ambiguous = true;
  }
  return ambiguous ? nullptr : match;
}
}  // namespace search

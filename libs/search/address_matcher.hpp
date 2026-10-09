#pragma once

#include "search/result.hpp"

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace search
{
// Recognize a bounded numeric house/street query, not a number followed by an arbitrary POI name.
bool IsAddressQuery(std::string const & query);

enum class AddressResultMatch
{
  None,
  Nearby,
  Interpolated,
  Exact
};
std::string DebugPrint(AddressResultMatch match);

struct AddressQuery
{
  uint64_t m_houseNumber = 0;
  std::string m_houseIdentifier;
  std::vector<std::string> m_streetAndLocalityTokens;
};

// Interpret formatting once per request and reuse the same tokens for ranking and validation.
std::optional<AddressQuery> ParseAddressQuery(std::string const & query);
AddressResultMatch GetAddressResultMatch(AddressQuery const & query, Result const & result,
                                         std::string const & expectedStreet = {});

// Shared classification for ranking and resolution; nearby addresses never count as the requested location.
AddressResultMatch GetAddressResultMatch(std::string const & query, Result const & result,
                                         std::string const & expectedStreet = {});

// Returns no match when distinct locations satisfy the query, rather than choosing an arbitrary address.
Result const * FindUniqueAddressResult(std::string const & query, Results const & results,
                                       std::string const & expectedStreet = {}, bool early = false);

// Prioritize mapped matches and nearby house numbers without inventing a missing address.
Results RankAddressResults(std::string const & query, Results const & results, std::string const & expectedStreet = {});

// Returns true only for a point result whose house number and street match the requested address.
bool IsAddressResultMatchingQuery(std::string const & query, Result const & result,
                                  std::string const & expectedStreet = {});

// Only mapped, fully qualified exact addresses can finish resolution before the search ends.
bool IsEarlyAddressResultMatchingQuery(std::string const & query, Result const & result,
                                       std::string const & expectedStreet);
}  // namespace search

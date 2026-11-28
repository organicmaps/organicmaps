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

// Parse a query into the mapped address components used for ranking and validation.
std::optional<AddressQuery> ParseAddressQuery(std::string const & query);
AddressResultMatch GetAddressResultMatch(AddressQuery const & query, Result const & result,
                                         std::string const & expectedStreet = {});

// Shared classification for ranking and resolution; nearby addresses never count as the requested location.
AddressResultMatch GetAddressResultMatch(std::string const & query, Result const & result,
                                         std::string const & expectedStreet = {});

// Returns no match when distinct locations satisfy the query, rather than choosing an arbitrary address.
Result const * FindUniqueAddressResult(std::string const & query, Results const & results,
                                       std::string const & expectedStreet = {});

struct AddressMatchRank
{
  AddressResultMatch m_match = AddressResultMatch::None;
  uint64_t m_numberDistance = 0;
};

// Classify an address and record the distance between nearby civic numbers for the ranker.
AddressMatchRank GetAddressMatchRank(AddressQuery const & query, Result const & result);

// Returns true only for a point result whose house number and street match the requested address.
bool IsAddressResultMatchingQuery(std::string const & query, Result const & result,
                                  std::string const & expectedStreet = {});

}  // namespace search

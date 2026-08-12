#pragma once

#include "search/result.hpp"

#include <string>

namespace search
{
// Replaces nearby house-number matches with an estimated address when at least two consistent
// same-street results provide enough evidence for interpolation or limited extrapolation.
Results MakeEstimatedAddressResults(std::string const & query, Results const & results);

// Returns true only for a point result whose house number and street match the requested address.
// Call this after MakeEstimatedAddressResults() so a validated estimate can match as well.
bool IsAddressResultMatchingQuery(std::string const & query, Result const & result);
}  // namespace search

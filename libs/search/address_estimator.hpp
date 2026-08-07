#pragma once

#include "search/result.hpp"

#include <string>

namespace search
{
// Replaces nearby house-number matches with an estimated address when at least two consistent
// same-street results provide enough evidence for interpolation or limited extrapolation.
Results MakeEstimatedAddressResults(std::string const & query, Results const & results);
}  // namespace search

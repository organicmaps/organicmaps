#pragma once

#include "indexer/string_slice.hpp"

#include "std/cstdint.hpp"
#include "std/string.hpp"

namespace search
{
/// If isPrefix is true returns true if some postcode starts with s.
/// If isPrefix is false returns true if s equals to some postcode.
bool LooksLikePostcode(StringSliceBase const & slice, bool isPrefix);
/// Splits s into tokens and call LooksLikePostcode(TokenSlice) on the result.
bool LooksLikePostcode(string const & s, bool isPrefix);

size_t GetMaxNumTokensInPostcode();
}  // namespace search

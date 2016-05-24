#pragma once

#include "indexer/string_slice.hpp"

#include "std/cstdint.hpp"
#include "std/string.hpp"

namespace search
{
bool LooksLikePostcode(StringSliceBase const & slice, bool handleAsPrefix);
/// Splits s into tokens and call LooksLikePostcode(TokenSlice) on the result.
/// If handleAsPrefix is true returns true if some postcode starts with s.
/// If handleAsPrefix is false returns true if s equals to some postcode.
bool LooksLikePostcode(string const & s, bool handleAsPrefix);

size_t GetMaxNumTokensInPostcode();
}  // namespace search

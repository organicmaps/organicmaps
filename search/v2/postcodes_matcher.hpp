#pragma once

#include "std/cstdint.hpp"
#include "std/string.hpp"

namespace search
{
namespace v2
{
class TokenSlice;

bool LooksLikePostcode(TokenSlice const & slice);
/// Splits s into tokens and call LooksLikePostcode(TokenSlice) on the result.
/// If checkPrefix is true returns true if some postcode starts with s.
/// If checkPrefix is false returns true if s equals to some postcode.
bool LooksLikePostcode(string const & s, bool checkPrefix);

size_t GetMaxNumTokensInPostcode();
}  // namespace v2
}  // namespace search

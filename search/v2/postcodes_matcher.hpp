#pragma once

#include "std/cstdint.hpp"

namespace search
{
namespace v2
{
class TokenSlice;

bool LooksLikePostcode(TokenSlice const & slice);

size_t GetMaxNumTokensInPostcode();
}  // namespace v2
}  // namespace search

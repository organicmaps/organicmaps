#pragma once

#include "std/cstdint.hpp"

namespace search
{
// todo(@m) We should not need that much.
size_t constexpr kMaxOpenFiles = 4000;

void ChangeMaxNumberOfOpenFiles(size_t n);
}  // namespace search

#pragma once

#include "std/cstdint.hpp"
#include "std/vector.hpp"

namespace il
{

void EncodePngToMemory(size_t width, size_t height,
                       vector<uint8_t> const & rgba,
                       vector<uint8_t> & out);

} // namespace il

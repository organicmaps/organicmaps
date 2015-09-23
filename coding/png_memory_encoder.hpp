#pragma once

#include "std/cstdint.hpp"
#include "std/vector.hpp"

namespace il
{

void EncodePngToMemory(uint32_t width, uint32_t height,
                       vector<uint8_t> const & rgba,
                       vector<uint8_t> & out);

} // namespace il

#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace base
{
// Builds suffix array for the string |s| and stores result in the
// |sa| array. Size of |sa| must be not less than |n|.
//
// Complexity: O(n) time and O(n) memory.
void Skew(size_t n, uint8_t const * s, size_t * sa);
void Skew(std::string const & s, std::vector<size_t> & sa);
}  // namespace base

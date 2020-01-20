#pragma once

#include <cstdint>
#include <string>

namespace coding
{
// Computes the Burrows-Wheeler transform of the string |s|, stores
// result in the string |r|. Note - the size of |r| must be |n|.
// Returns the index of the original string among the all sorted
// rotations of the |s|.
//
// *NOTE* in contrast to popular explanations of BWT, we do not append
// to |s| trailing '$' that is less than any other character in |s|.
// The reason is that |s| can be an arbitrary byte string, with zero
// bytes inside, so implementation of this trailing '$' is expensive,
// and, actually, not needed.
//
// For example, if |s| is "abaaba", canonical BWT is:
//
//   Sorted rotations:       canonical BWT:
//   $abaaba                 a
//   a$abaab                 b
//   aaba$ab                 b
//   aba$aba                 a
// * abaaba$                 $
//   ba$abaa                 a
//   baaba$a                 a
//
// where '*' denotes original string.
//
// Our implementation will sort rotations in a way as there is an
// implicit '$' that is less than any other byte in |s|, but does not
// return this '$'. Therefore, the order of rotations will be the same
// as above, without the first '$abaaba':
//
//   Sorted rotations:      ours BWT:
//   aabaab                 b
//   aabaab                 b
//   abaaba                 a
// * abaaba                 a
//   baabaa                 a
//   baabaa                 a
//
// where '*' denotes the index of original string. As one can see,
// there are two 'abaaba' strings, but as mentioned, rotations are
// sorted like there is an implicit '$' at the end of the original
// string. It's possible to get from "ours BWT" to the "original BWT",
// see the code for details.
//
// Complexity: O(n) time and O(n) memory.
size_t BWT(size_t n, uint8_t const * s, uint8_t * r);
size_t BWT(std::string const & s, std::string & r);

// Inverse Burrows-Wheeler transform.
//
// Complexity: O(n) time and O(n) memory.
void RevBWT(size_t n, size_t start, uint8_t const * s, uint8_t * r);
void RevBWT(size_t start, std::string const & s, std::string & r);
}  // namespace coding

#pragma once

#include "std/string.hpp"

namespace base64
{
string Encode(string const & bytesToEncode);
string Decode(string const & base64CharsToDecode);
}  // namespace base64

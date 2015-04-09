#pragma once

#include "std/vector.hpp"
#include "std/string.hpp"

namespace base64
{

string Encode(string const & bytesToEncode);
string Decode(string const & base64CharsToDecode);

}

/// This namespace contains historically invalid implementation of base64,
/// but it's still needed for production code
namespace base64_for_user_ids
{

string encode(string rawBytes);
string decode(string const & base64Chars);

}

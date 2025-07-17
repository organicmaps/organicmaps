#pragma once

#include <string>

namespace base64
{
std::string Encode(std::string_view bytesToEncode);
std::string Decode(std::string const & base64CharsToDecode);
}  // namespace base64

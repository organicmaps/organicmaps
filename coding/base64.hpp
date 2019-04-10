#pragma once

#include <string>

namespace base64
{
std::string Encode(std::string const & bytesToEncode);
std::string Decode(std::string const & base64CharsToDecode);
}  // namespace base64

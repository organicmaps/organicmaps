#include "coding/sha2.hpp"
#include "coding/hex.hpp"

#include "base/macros.hpp"

#include "3party/tomcrypt/src/headers/tomcrypt.h"

namespace sha2
{
  string digest256(char const * data, size_t dataSize, bool returnAsHexString)
  {
    hash_state md;
    unsigned char out[256/8] = { 0 };
    if (CRYPT_OK == sha256_init(&md)
        && CRYPT_OK == sha256_process(&md, reinterpret_cast<unsigned char const *>(data), dataSize)
        && CRYPT_OK == sha256_done(&md, out))
    {
      if (returnAsHexString)
        return ToHex(string(reinterpret_cast<char const *>(out), ARRAY_SIZE(out)));
      else
        return string(reinterpret_cast<char const *>(out), ARRAY_SIZE(out));
    }
    return string();
  }
}

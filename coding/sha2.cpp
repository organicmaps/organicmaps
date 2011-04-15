#include "sha2.hpp"
#include "hex.hpp"

#include "../base/macros.hpp"

#include <tomcrypt.h>

namespace sha2
{
  string digest224(char const * data, size_t dataSize, bool returnAsHexString)
  {
    hash_state md;
    unsigned char out[224/8] = { 0 };
    if (CRYPT_OK == sha224_init(&md)
        && CRYPT_OK == sha224_process(&md, reinterpret_cast<unsigned char const *>(data), dataSize)
        && CRYPT_OK == sha224_done(&md, out))
    {
      if (returnAsHexString)
        return ToHex(string(reinterpret_cast<char const *>(out), ARRAY_SIZE(out)));
      else
        return string(reinterpret_cast<char const *>(out), ARRAY_SIZE(out));
    }
    return string();
  }

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

  string digest384(char const * data, size_t dataSize, bool returnAsHexString)
  {
    hash_state md;
    unsigned char out[384/8] = { 0 };
    if (CRYPT_OK == sha384_init(&md)
        && CRYPT_OK == sha384_process(&md, reinterpret_cast<unsigned char const *>(data), dataSize)
        && CRYPT_OK == sha384_done(&md, out))
    {
      if (returnAsHexString)
        return ToHex(string(reinterpret_cast<char const *>(out), ARRAY_SIZE(out)));
      else
        return string(reinterpret_cast<char const *>(out), ARRAY_SIZE(out));
    }
    return string();
  }

  string digest512(char const * data, size_t dataSize, bool returnAsHexString)
  {
    hash_state md;
    unsigned char out[512/8] = { 0 };
    if (CRYPT_OK == sha512_init(&md)
        && CRYPT_OK == sha512_process(&md, reinterpret_cast<unsigned char const *>(data), dataSize)
        && CRYPT_OK == sha512_done(&md, out))
    {
      if (returnAsHexString)
        return ToHex(string(reinterpret_cast<char const *>(out), ARRAY_SIZE(out)));
      else
        return string(reinterpret_cast<char const *>(out), ARRAY_SIZE(out));
    }
    return string();
  }
}


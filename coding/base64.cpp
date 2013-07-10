#include "base64.hpp"

#include "../3party/tomcrypt/src/headers/tomcrypt.h"
#include "../3party/tomcrypt/src/headers/tomcrypt_misc.h"
#include "../base/assert.hpp"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wreorder"
#include <boost/archive/iterators/base64_from_binary.hpp>
#include <boost/archive/iterators/binary_from_base64.hpp>
#include <boost/archive/iterators/transform_width.hpp>
#pragma GCC diagnostic pop

using namespace boost::archive::iterators;
typedef base64_from_binary<transform_width<string::const_iterator, 6, 8> > base64_t;
typedef transform_width<binary_from_base64<string::const_iterator>, 8, 6 > binary_t;

/// This namespace contains historically invalid implementation of base64,
/// but it's still needed for production code
namespace base64_for_user_ids
{
  string encode(string rawBytes)
  {
    // http://boost.2283326.n4.nabble.com/boost-archive-iterators-base64-from-binary-td2589980.html
    switch (rawBytes.size() % 3)
    {
    case 1:
      rawBytes.push_back('0');
    case 2:
      rawBytes.push_back('0');
      break;
    }

    return string(base64_t(rawBytes.begin()), base64_t(rawBytes.end()));
  }

  string decode(string const & base64Chars)
  {
    if (base64Chars.empty())
      return string();
    // minus 1 needed to avoid finishing zero in a string
    return string(binary_t(base64Chars.begin()), binary_t(base64Chars.begin() + base64Chars.size() - 1));
  }
}

namespace base64
{

string Encode(string const & bytesToEncode)
{
  string result;
  long unsigned int resSize = (bytesToEncode.size() + 3) * 4 / 3;
  // Raw new/delete because we don't throw exceptions inbetween
  unsigned char * buffer = new unsigned char[resSize];
  if (CRYPT_OK == base64_encode(reinterpret_cast<unsigned char const *>(bytesToEncode.data()),
                                bytesToEncode.size(),
                                buffer,
                                &resSize))
    result.assign(reinterpret_cast<char const *>(buffer), resSize);
  else
    ASSERT(false, ("It should work!"));

  delete[] buffer;
  return result;
}

string Decode(string const & base64CharsToDecode)
{
  string result;
  long unsigned int resSize = base64CharsToDecode.size() * 3 / 4 + 2;
  // Raw new/delete because we don't throw exceptions inbetween
  unsigned char * buffer = new unsigned char[resSize];
  if (CRYPT_OK == base64_decode(reinterpret_cast<unsigned char const *>(base64CharsToDecode.data()),
                                base64CharsToDecode.size(),
                                buffer,
                                &resSize))
    result.assign(reinterpret_cast<char const *>(buffer), resSize);
  else
    ASSERT(false, ("It should work!"));

  delete[] buffer;
  return result;
}

}

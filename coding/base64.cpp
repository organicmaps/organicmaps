#include "base64.hpp"

#include <boost/archive/iterators/base64_from_binary.hpp>
#include <boost/archive/iterators/binary_from_base64.hpp>
#include <boost/archive/iterators/transform_width.hpp>

using namespace boost::archive::iterators;
typedef base64_from_binary<transform_width<string::const_iterator, 6, 8> > base64_t;
typedef transform_width<binary_from_base64<string::const_iterator>, 8, 6 > binary_t;

namespace base64
{
  string encode(string const & rawBytes)
  {
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

#include "ge0/url_generator.hpp"

#include "base/assert.hpp"

#include <cmath>

using namespace std;

namespace
{
// Replaces ' ' with '_' and vice versa.
string TransformName(string const & s)
{
  string result = s;
  for (auto & c : result)
  {
    if (c == ' ')
      c = '_';
    else if (c == '_')
      c = ' ';
  }
  return result;
}

// URL-encodes string |s|.
// URL restricted / unsafe / unwise characters are %-encoded.
// See rfc3986, rfc1738, rfc2396.
//
// Not compatible with the url encode function from coding/.
string UrlEncodeString(string const & s)
{
  string result;
  result.reserve(s.size() * 3 + 1);
  for (size_t i = 0; i < s.size(); ++i)
  {
    auto const c = static_cast<unsigned char>(s[i]);
    switch (c)
    {
    case 0x00: case 0x01: case 0x02: case 0x03: case 0x04: case 0x05: case 0x06: case 0x07:
    case 0x08: case 0x09: case 0x0A: case 0x0B: case 0x0C: case 0x0D: case 0x0E: case 0x0F:
    case 0x10: case 0x11: case 0x12: case 0x13: case 0x14: case 0x15: case 0x16: case 0x17:
    case 0x18: case 0x19: case 0x1A: case 0x1B: case 0x1C: case 0x1D: case 0x1E: case 0x1F:
    case 0x7F:
    case ' ':
    case '<':
    case '>':
    case '#':
    case '%':
    case '"':
    case '!':
    case '*':
    case '\'':
    case '(':
    case ')':
    case ';':
    case ':':
    case '@':
    case '&':
    case '=':
    case '+':
    case '$':
    case ',':
    case '/':
    case '?':
    case '[':
    case ']':
    case '{':
    case '}':
    case '|':
    case '^':
    case '`':
      result += '%';
      result += "0123456789ABCDEF"[c >> 4];
      result += "0123456789ABCDEF"[c & 15];
      break;
    default:
      result += s[i];
    }
  }
  return result;
}
}  // namespace

namespace ge0
{
string GenerateShortShowMapUrl(double lat, double lon, double zoom, string const & name)
{
  string urlPrefix = "ge0://ZCoordba64";

  int const zoomI = (zoom <= 4 ? 0 : (zoom >= 19.75 ? 63 : static_cast<int>((zoom - 4) * 4)));
  urlPrefix[6] = Base64Char(zoomI);

  LatLonToString(lat, lon, urlPrefix.data() + 7, 9);

  string result = urlPrefix;
  if (!name.empty())
  {
    result += "/";
    result += UrlEncodeString(TransformName(name));
  }

  return result;
}

char Base64Char(int x)
{
  CHECK_GREATER_OR_EQUAL(x, 0, ());
  CHECK_LESS(x, 64, ());
  return "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_"[x];
}

// Map latitude: [-90, 90] -> [0, maxValue]
int LatToInt(double lat, int maxValue)
{
  // M = maxValue, L = maxValue-1
  // lat: -90                        90
  //   x:  0     1     2       L     M
  //       |--+--|--+--|--...--|--+--|
  //       000111111222222...LLLLLMMMM

  double const x = (lat + 90.0) / 180.0 * maxValue;
  return x < 0 ? 0 : (x > maxValue ? maxValue : static_cast<int>(x + 0.5));
}

// Make lon in [-180, 180)
double LonIn180180(double lon)
{
  if (lon >= 0)
    return fmod(lon + 180.0, 360.0) - 180.0;

  // Handle the case of l = -180
  double const l = fmod(lon - 180.0, 360.0) + 180.0;
  return l < 180.0 ? l : l - 360.0;
}

// Map longitude: [-180, 180) -> [0, maxValue]
int LonToInt(double lon, int maxValue)
{
  double const x = (LonIn180180(lon) + 180.0) / 360.0 * (maxValue + 1.0) + 0.5;
  return (x <= 0 || x >= maxValue + 1) ? 0 : static_cast<int>(x);
}

void LatLonToString(double lat, double lon, char * s, size_t nBytes)
{
  if (nBytes > kMaxPointBytes)
    nBytes = kMaxPointBytes;

  int const latI = LatToInt(lat, (1 << kMaxCoordBits) - 1);
  int const lonI = LonToInt(lon, (1 << kMaxCoordBits) - 1);

  size_t i;
  int shift;
  for (i = 0, shift = kMaxCoordBits - 3; i < nBytes; ++i, shift -= 3)
  {
    int const latBits = latI >> shift & 7;
    int const lonBits = lonI >> shift & 7;

    int const nextByte =
      (latBits >> 2 & 1) << 5 |
      (lonBits >> 2 & 1) << 4 |
      (latBits >> 1 & 1) << 3 |
      (lonBits >> 1 & 1) << 2 |
      (latBits      & 1) << 1 |
      (lonBits      & 1);

    s[i] = Base64Char(nextByte);
  }
}
}  // namespace ge0

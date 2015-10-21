#include "api-client.h"
#include <assert.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>

// Max number of base64 bytes to encode a geo point.
#define MAPSWITHME_MAX_POINT_BYTES 10
#define MAPSWITHME_MAX_COORD_BITS (MAPSWITHME_MAX_POINT_BYTES * 3)

char MapsWithMe_Base64Char(int x)
{
  assert(x >= 0 && x < 64);
  return "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_"[x];
}

// Map latitude: [-90, 90] -> [0, maxValue]
int MapsWithMe_LatToInt(double lat, int maxValue)
{
  // M = maxValue, L = maxValue-1
  // lat: -90                        90
  //   x:  0     1     2       L     M
  //       |--+--|--+--|--...--|--+--|
  //       000111111222222...LLLLLMMMM

  double const x = (lat + 90.0) / 180.0 * maxValue;
  return (x < 0 ? 0 : (x > maxValue ? maxValue : (int)(x + 0.5)));
}

// Make lon in [-180, 180)
double MapsWithMe_LonIn180180(double lon)
{
  if (lon >= 0)
    return fmod(lon + 180.0, 360.0) - 180.0;
  else
  {
    // Handle the case of l = -180
    double const l = fmod(lon - 180.0, 360.0) + 180.0;
    return l < 180.0 ? l : l - 360.0;
  }
}

// Map longitude: [-180, 180) -> [0, maxValue]
int MapsWithMe_LonToInt(double lon, int maxValue)
{
  double const x = (MapsWithMe_LonIn180180(lon) + 180.0) / 360.0 * (maxValue + 1.0) + 0.5;
  return (x <= 0 || x >= maxValue + 1) ? 0 : (int)x;
}

void MapsWithMe_LatLonToString(double lat, double lon, char * s, int nBytes)
{
  if (nBytes > MAPSWITHME_MAX_POINT_BYTES)
    nBytes = MAPSWITHME_MAX_POINT_BYTES;

  int const latI = MapsWithMe_LatToInt(lat, (1 << MAPSWITHME_MAX_COORD_BITS) - 1);
  int const lonI = MapsWithMe_LonToInt(lon, (1 << MAPSWITHME_MAX_COORD_BITS) - 1);

  int i, shift;
  for (i = 0, shift = MAPSWITHME_MAX_COORD_BITS - 3; i < nBytes; ++i, shift -= 3)
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

    s[i] = MapsWithMe_Base64Char(nextByte);
  }
}

// Replaces ' ' with '_' and vice versa.
void MapsWithMe_TransformName(char * s)
{
  for (; *s != 0; ++s)
  {
    if (*s == ' ')
      *s = '_';
    else if (*s == '_')
      *s = ' ';
  }
}

// URL Encode string s.
// Allocates memory that should be freed.
// Returns the lenghts of the resulting string in bytes including terminating 0.
// URL restricted / unsafe / unwise characters are %-encoded.
// See rfc3986, rfc1738, rfc2396.
size_t MapsWithMe_UrlEncodeString(char const * s, size_t size, char ** res)
{
  size_t i;
  char * out;
  *res = malloc(size * 3 + 1);
  out = *res;
  for (i = 0; i < size; ++i)
  {
    unsigned char c = (unsigned char)(s[i]);
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
      *(out++) = '%';
      *(out++) = "0123456789ABCDEF"[c >> 4];
      *(out++) = "0123456789ABCDEF"[c & 15];
      break;
    default:
      *(out++) = s[i];
    }
  }
  *(out++) = 0;
  return out - *res - 1;
}

// Append s to buf (is there is space). Increment *bytesAppended by size.
void MapsWithMe_AppendString(char * buf, int bufSize, int * bytesAppended, char const * s, size_t size)
{
  size_t const bytesAvailable = bufSize - *bytesAppended;
  if (bytesAvailable > 0)
    memcpy(buf + *bytesAppended, s, size < bytesAvailable ? size : bytesAvailable);

  *bytesAppended += size;
}

int MapsWithMe_GetMaxBufferSize(int nameSize)
{
  return ((nameSize == 0) ? 17 : 17 + 3 * nameSize + 1);
}

int MapsWithMe_GenShortShowMapUrl(double lat, double lon, double zoom, char const * name, char * buf, int bufSize)
{
  // URL format:
  //
  //       +------------------  1 byte: zoom level
  //       |+-------+---------  9 bytes: lat,lon
  //       ||       | +--+----  Variable number of bytes: point name
  //       ||       | |  |
  // ge0://ZCoordba64/Name

  int fullUrlSize = 0;

  char urlPrefix[] = "ge0://ZCoord6789";

  int const zoomI = (zoom <= 4 ? 0 : (zoom >= 19.75 ? 63 : (int) ((zoom - 4) * 4)));
  urlPrefix[6] = MapsWithMe_Base64Char(zoomI);

  MapsWithMe_LatLonToString(lat, lon, urlPrefix + 7, 9);

  MapsWithMe_AppendString(buf, bufSize, &fullUrlSize, urlPrefix, 16);

  if (name != 0 && name[0] != 0)
  {
    MapsWithMe_AppendString(buf, bufSize, &fullUrlSize, "/", 1);

    char * newName = strdup(name);
    MapsWithMe_TransformName(newName);

    char * encName;
    size_t const encNameSize = MapsWithMe_UrlEncodeString(newName, strlen(newName), &encName);

    MapsWithMe_AppendString(buf, bufSize, &fullUrlSize, encName, encNameSize);

    free(encName);
    free(newName);
  }

  return fullUrlSize;
}

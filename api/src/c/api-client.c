#include "api-client.h"
#include <assert.h>
#include <math.h>

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

  for (int i = 0, shift = MAPSWITHME_MAX_COORD_BITS - 3; i < nBytes; ++i, shift -= 3)
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

int MapsWithMe_GenShortShowMapUrl(double lat, double lon, double zoomLevel, char const * name, char * buf, int bufSize)
{
  // @TODO: Implement MapsWithMe_GenShortShowMapUrl().
  // @TODO: Escape URL-unfriendly characters: ! * ' ( ) ; : @ & = + $ , / ? % # [ ]

  return 0;
}

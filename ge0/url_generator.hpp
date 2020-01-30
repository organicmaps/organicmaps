#pragma once

#include <string>

namespace ge0
{
// Max number of base64 bytes to encode a geo point.
inline static int const kMaxPointBytes = 10;
inline static int const kMaxCoordBits = kMaxPointBytes * 3;

// Generates a short url.
//
// URL format:
//
//       +------------------  1 byte: zoom level
//       |+-------+---------  9 bytes: lat,lon
//       ||       | +--+----  Variable number of bytes: point name
//       ||       | |  |
// ge0://ZCoordba64/Name
std::string GenerateShortShowMapUrl(double lat, double lon, double zoomLevel, std::string const & name);

// Exposed for testing.
char Base64Char(int x);
int LatToInt(double lat, int maxValue);
double LonIn180180(double lon);
int LonToInt(double lon, int maxValue);
void LatLonToString(double lat, double lon, char * s, size_t nBytes);

/* @TODO(melnichek): Finish URL Scheme API implementation and uncomment.
typedef struct
{
  double lat;
  double lon;
} Coordinate;

typedef struct
{
  double lat;
  double lon;
  char const * name;
  char const * id;
} Pin;

typedef struct
{
  char const * toolbarTitle;
  char const * backButtonTitle;
  char const * backButtonUrl;
  char const * backPinUrl;

  Pin const * pins;
  int pinsSize;

  Coordinate viewportCenter;
  double viewportZoomLevel;
} ShowMapRequest;

// Generate a url to shows a map view.
char * GenShowMapUrl(ShowMapRequest const * request);
*/
}  // namespace ge0

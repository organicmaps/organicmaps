#pragma once

namespace ge0
{
// Max number of base64 bytes to encode a geo point.
inline static int const kMaxPointBytes = 10;
inline static int const kMaxCoordBits = kMaxPointBytes * 3;

char Base64Char(int x);
int LatToInt(double lat, int maxValue);
double LonIn180180(double lon);
int LonToInt(double lon, int maxValue);
void LatLonToString(double lat, double lon, char * s, int nBytes);

// Helper function to calculate maximum buffer size for
// GenShortShowMapUrl (with null-terminator).
int GetMaxBufferSize(int nameSize);

// Helper method to generate short url.
// Returns the number of bytes required to fit the whole URL or an error_code < 0 on error.
int GenShortShowMapUrl(double lat, double lon, double zoomLevel, char const * name, char * buf,
                       int bufSize);

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

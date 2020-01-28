#ifndef MAPSWITHME_API_CLIENT
#define MAPSWITHME_API_CLIENT

#ifdef __cplusplus
extern "C" {
#endif

// Max number of base64 bytes to encode a geo point.
#define MAPSWITHME_MAX_POINT_BYTES 10
#define MAPSWITHME_MAX_COORD_BITS (MAPSWITHME_MAX_POINT_BYTES * 3)

char MapsWithMe_Base64Char(int x);
int MapsWithMe_LatToInt(double lat, int maxValue);
double MapsWithMe_LonIn180180(double lon);
int MapsWithMe_LonToInt(double lon, int maxValue);
void MapsWithMe_LatLonToString(double lat, double lon, char * s, int nBytes);

#ifdef __cplusplus
}  // Closing brace for extern "C"
#endif


#ifdef __cplusplus
extern "C" {
#endif

/* @TODO: Finish URL Scheme API implementation and uncomment.
typedef struct
{
  double lat;
  double lon;
} MapsWithMe_Coordinate;

typedef struct
{
  double lat;
  double lon;
  char const * name;
  char const * id;
} MapsWithMe_Pin;

typedef struct
{
  char const * toolbarTitle;
  char const * backButtonTitle;
  char const * backButtonUrl;
  char const * backPinUrl;

  MapsWithMe_Pin const * pins;
  int pinsSize;

  MapsWithMe_Coordinate viewportCenter;
  double viewportZoomLevel;
} MapsWithMe_ShowMapRequest;

// Generate a url to shows a map view.
char * MapsWithMe_GenShowMapUrl(MapsWithMe_ShowMapRequest const * request);
*/

// Helper function to calculate maximum buffer size for
// MapsWithMe_GenShortShowMapUrl (with null-terminator).
int MapsWithMe_GetMaxBufferSize(int nameSize);

// Helper method to generate short url.
// Returns the number of bytes required to fit the whole URL or an error_code < 0 on error.
int MapsWithMe_GenShortShowMapUrl(double lat, double lon, double zoomLevel, char const * name, char * buf, int bufSize);

#ifdef __cplusplus
}  // Closing brace for extern "C"
#endif

#endif  // MAPSWITHME_API_CLIENT

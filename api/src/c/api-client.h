#ifndef MAPSWITHME_API_CLIENT
#define MAPSWITHME_API_CLIENT

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

// Helper method to generate short url.
char * MapsWithMe_GenShortShowMapUrl(double lat, double lon, double zoomLevel, char const * name);

#ifdef __cplusplus
}  // Closing brace for extern "C"
#endif

#endif  // MAPSWITHME_API_CLIENT

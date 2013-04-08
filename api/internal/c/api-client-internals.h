#ifndef MAPSWITHME_API_INTERNAL_API_CLIENT_INTERNALS
#define MAPSWITHME_API_INTERNAL_API_CLIENT_INTERNALS

#ifdef __cplusplus
extern "C" {
#endif

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


#endif  // MAPSWITHME_API_INTERNAL_API_CLIENT_INTERNALS

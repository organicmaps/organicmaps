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
// om://ZCoordba64/Name
std::string GenerateShortShowMapUrl(double lat, double lon, double zoomLevel, std::string const & name);

// Generates a geo: uri.
//
// - https://datatracker.ietf.org/doc/html/rfc5870
// - https://developer.android.com/guide/components/intents-common#Maps
// - https://developers.google.com/maps/documentation/urls/android-intents
//
// URL format:
//
//     +--------------------------------  lat
//     |            +-------------------- lon
//     |            |               +---- zoom
//     |            |               | +-- url-encoded name
//     |            |               | |
//     |            |               | |
// geo:54.683486138,25.289361259&z=14(Forto%20dvaras)
std::string GenerateGeoUri(double lat, double lon, double zoom, std::string const & name);

// Exposed for testing.
char Base64Char(int x);
int LatToInt(double lat, int maxValue);
double LonIn180180(double lon);
int LonToInt(double lon, int maxValue);
void LatLonToString(double lat, double lon, char * s, size_t nBytes);
}  // namespace ge0

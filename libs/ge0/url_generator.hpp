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

// Same ge0 short url, but with the "https://omaps.app/" prefix, so it also opens the web landing
// page in a browser. This is the format shared by share::Build today - see the switch note there.
std::string GenerateHttpShowMapUrl(double lat, double lon, double zoomLevel, std::string const & name);

// Generates a shareable https://omaps.app/ url with human-readable decimal coordinates.
//
// URL format:
//   https://omaps.app/<lat>,<lon>[/<name>]?z=<zoom>
//
// Unlike the ge0 short url above, the coordinates stay readable and copy/paste-able (e.g. into
// Google/Apple Maps or a geo: uri). Coordinates use 6 decimal places (matches
// CoordinatesFormat::LatLonDecimal). Zoom is emitted as a "?z=" query parameter when > 0.
//
// Two decoders read it: Ge0Parser::ParseClearCoordinates in the app and the web landing page
// (organicmaps/url-processor). They agree on this exact form but not beyond it - the app accepts
// only it, while the web regex also matches other separators and coordinates that do not start the
// path, and rejects the +-90/+-180 corners the app allows. Keep this generator inside the overlap.
//
// NOT shared yet: apps released before Ge0Parser::ParseClearCoordinates shipped reject such links
// (see the switch note in share::Build). Used by tests and by the future generator switch.
std::string GenerateClearShowMapUrl(double lat, double lon, int zoom, std::string const & name);

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

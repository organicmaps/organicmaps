#include "ge0_parser.hpp"
#include "url_api.hpp"
#include "../api/internal/c/api-client-internals.h"
#include "../coding/url_encode.hpp"
#include "../base/math.hpp"
#include "../indexer/mercator.hpp"

static const int ZOOM_POSITION = 6;

url_api::Ge0Parser::Ge0Parser()
{
  for (size_t i = 0; i < 256; ++i)
    m_base64ReverseCharTable[i] = 255;
  for (uint8_t i = 0; i < 64; ++i)
  {
    char c = MapsWithMe_Base64Char(i);
    m_base64ReverseCharTable[static_cast<uint8_t>(c)] = i;
  }
}

bool url_api::Ge0Parser::Parse(string const & url, Request & request)
{
  // URL format:
  //
  //       +------------------  1 byte: zoom level
  //       |+-------+---------  9 bytes: lat,lon
  //       ||       | +--+----  Variable number of bytes: point name
  //       ||       | |  |
  // ge0://ZCoordba64/Name

  request.Clear();
  if (url.size() < 16 || url.substr(0, 6) != "ge0://")
    return false;

  uint8_t const zoomI = DecodeBase64Char(url[ZOOM_POSITION]);
  if (zoomI > 63)
    return false;
  request.m_viewportZoomLevel = DecodeZoom(zoomI);

  request.m_points.push_back(url_api::Point());
  url_api::Point & newPt = request.m_points.back();

  DecodeLatLon(url.substr(7, 9), newPt.m_lat, newPt.m_lon);

  ASSERT(MercatorBounds::ValidLon(newPt.m_lon), (newPt.m_lon));
  ASSERT(MercatorBounds::ValidLat(newPt.m_lat), (newPt.m_lat));

  request.m_viewportLat = newPt.m_lat;
  request.m_viewportLon = newPt.m_lon;

  if (url.size() >= NAME_POSITON_IN_URL)
    newPt.m_name = DecodeName(url.substr(NAME_POSITON_IN_URL, url.size() - NAME_POSITON_IN_URL));
  return true;
}

uint8_t url_api::Ge0Parser::DecodeBase64Char(char const c)
{
  return m_base64ReverseCharTable[static_cast<uint8_t>(c)];
}

double url_api::Ge0Parser::DecodeZoom(uint8_t const zoomByte)
{
  //Coding zoom -  int newZoom = ((oldZoom - 4) * 4)
  return static_cast<double>(zoomByte) / 4 + 4;
}

void url_api::Ge0Parser::DecodeLatLon(string const & url, double & lat, double & lon)
{
  int latInt = 0, lonInt = 0;
  DecodeLatLonToInt(url, latInt, lonInt, url.size());
  lat = DecodeLatFromInt(latInt, (1 << MAPSWITHME_MAX_COORD_BITS) - 1);
  lon = DecodeLonFromInt(lonInt, (1 << MAPSWITHME_MAX_COORD_BITS) - 1);
}

void url_api::Ge0Parser::DecodeLatLonToInt(string const & url, int & lat, int & lon, int const bytes)
{
  for(int i = 0, shift = MAPSWITHME_MAX_COORD_BITS - 3; i < bytes; ++i, shift -= 3)
  {
    const uint8_t a = DecodeBase64Char(url[i]);
    const int lat1 =  (((a >> 5) & 1) << 2 |
                 ((a >> 3) & 1) << 1 |
                 ((a >> 1) & 1));
    const int lon1 =  (((a >> 4) & 1) << 2 |
                 ((a >> 2) & 1) << 1 |
                        (a & 1));
    lat |= lat1 << shift;
    lon |= lon1 << shift;
  }
  const double middleOfSquare = 1 << (3 * (MAPSWITHME_MAX_POINT_BYTES - bytes) - 1);
  lat += middleOfSquare;
  lon += middleOfSquare;
}

double url_api::Ge0Parser::DecodeLatFromInt(int const lat, int const maxValue)
{
  return static_cast<double>(lat) / maxValue * 180 - 90;
}

double url_api::Ge0Parser::DecodeLonFromInt(int const lon, int const maxValue)
{
  return static_cast<double>(lon) / (maxValue + 1.0) * 360.0 - 180;
}

string url_api::Ge0Parser::DecodeName(string const & name)
{
  string resultName = name;
  ValidateName(resultName);
  resultName = UrlDecode(resultName);
  SpacesToUnderscore(resultName);
  return resultName;
}

void url_api::Ge0Parser::SpacesToUnderscore(string & name)
{
  for (size_t i = 0; i < name.size(); ++i)
    if (name[i] == ' ')
      name[i] = '_';
    else if (name[i] == '_')
      name[i] = ' ';
}

void url_api::Ge0Parser::ValidateName(string & name)
{
  if (name.empty())
    return;
  for (size_t i = 0; i + 2 < name.size(); ++i)
  {
    if (name[i] == '%' && (!IsHexChar(name[i + 1]) || !IsHexChar(name[i + 2])))
    {
      name.resize(i);
      return;
    }
  }
  if (name[name.size() - 1] == '%')
    name.resize(name.size() - 1);
  else if (name.size() > 1 && name[name.size() - 2] == '%')
    name.resize(name.size() - 2);
}

bool url_api::Ge0Parser::IsHexChar(char const a)
{
  return ((a >= '0' && a <= '9') || (a >= 'A' && a <= 'F') || (a >= 'a' && a <= 'f'));
}

#include "ge0/parser.hpp"

#include "ge0/url_generator.hpp"

#include "coding/url.hpp"

#include "geometry/mercator.hpp"

#include "base/assert.hpp"
#include "base/math.hpp"
#include "base/string_utils.hpp"

#include <algorithm>

using namespace std;

namespace ge0
{
Ge0Parser::Ge0Parser()
{
  for (size_t i = 0; i < 256; ++i)
    m_base64ReverseCharTable[i] = 255;
  for (uint8_t i = 0; i < 64; ++i)
  {
    char c = Base64Char(i);
    m_base64ReverseCharTable[static_cast<uint8_t>(c)] = i;
  }
}

bool Ge0Parser::Parse(string const & url, double & outLat, double & outLon, std::string & outName, double & outZoomLevel)
{
  // Original URL format:
  //
  //       +------------------  1 byte: zoom level
  //       |+-------+---------  9 bytes: lat,lon
  //       ||       | +--+----  Variable number of bytes: point name
  //       ||       | |  |
  // ge0://ZCoordba64/Name

  // Alternative format (differs only in the prefix):
  // http://ge0.me/ZCoordba64/Name

  for (string const & prefix : {"ge0://", "http://ge0.me/", "https://ge0.me/"})
  {
    if (strings::StartsWith(url, prefix))
      return ParseAfterPrefix(url, prefix.size(), outLat, outLon, outName, outZoomLevel);
  }

  return false;
}

bool Ge0Parser::ParseAfterPrefix(string const & url, size_t from, double & outLat, double & outLon,
                                 std::string & outName, double & outZoomLevel)
{
  size_t const kEncodedZoomAndCoordinatesLength = 10;
  if (url.size() < from + kEncodedZoomAndCoordinatesLength)
    return false;

  size_t const kMaxNameLength = 256;

  size_t const posZoom = from;
  size_t const posLatLon = posZoom + 1;
  size_t const posName = from + kEncodedZoomAndCoordinatesLength + 1;
  size_t const lengthLatLon = posName - posLatLon - 1;

  uint8_t const zoomI = DecodeBase64Char(url[posZoom]);
  if (zoomI >= 64)
    return false;
  outZoomLevel = DecodeZoom(zoomI);

  if (!DecodeLatLon(url.substr(posLatLon, lengthLatLon), outLat, outLon))
    return false;

  ASSERT(mercator::ValidLon(outLon), (outLon));
  ASSERT(mercator::ValidLat(outLat), (outLat));

  if (url.size() >= posName)
  {
    CHECK_GREATER(posName, 0, ());
    if (url[posName - 1] != '/')
      return false;
    outName = DecodeName(url.substr(posName, min(url.size() - posName, kMaxNameLength)));
  }

  return true;
}

uint8_t Ge0Parser::DecodeBase64Char(char const c)
{
  return m_base64ReverseCharTable[static_cast<uint8_t>(c)];
}

double Ge0Parser::DecodeZoom(uint8_t const zoomByte)
{
  // Coding zoom -  int newZoom = ((oldZoom - 4) * 4)
  return static_cast<double>(zoomByte) / 4 + 4;
}

bool Ge0Parser::DecodeLatLon(string const & s, double & lat, double & lon)
{
  int latInt = 0;
  int lonInt = 0;
  if (!DecodeLatLonToInt(s, latInt, lonInt))
    return false;

  lat = DecodeLatFromInt(latInt, (1 << kMaxCoordBits) - 1);
  lon = DecodeLonFromInt(lonInt, (1 << kMaxCoordBits) - 1);
  return true;
}

bool Ge0Parser::DecodeLatLonToInt(string const & s, int & lat, int & lon)
{
  int shift = kMaxCoordBits - 3;
  for (size_t i = 0; i < s.size(); ++i, shift -= 3)
  {
    uint8_t const a = DecodeBase64Char(s[i]);
    if (a >= 64)
      return false;

    int const lat1 = (((a >> 5) & 1) << 2 | ((a >> 3) & 1) << 1 | ((a >> 1) & 1));
    int const lon1 = (((a >> 4) & 1) << 2 | ((a >> 2) & 1) << 1 | (a & 1));
    lat |= lat1 << shift;
    lon |= lon1 << shift;
  }
  double const middleOfSquare = 1 << (3 * (kMaxPointBytes - s.size()) - 1);
  lat += middleOfSquare;
  lon += middleOfSquare;
  return true;
}

double Ge0Parser::DecodeLatFromInt(int const lat, int const maxValue)
{
  return static_cast<double>(lat) / maxValue * 180 - 90;
}

double Ge0Parser::DecodeLonFromInt(int const lon, int const maxValue)
{
  return static_cast<double>(lon) / (maxValue + 1.0) * 360.0 - 180;
}

string Ge0Parser::DecodeName(string name)
{
  ValidateName(name);
  name = url::UrlDecode(name);
  SpacesToUnderscore(name);
  return name;
}

void Ge0Parser::SpacesToUnderscore(string & name)
{
  for (size_t i = 0; i < name.size(); ++i)
  {
    if (name[i] == ' ')
      name[i] = '_';
    else if (name[i] == '_')
      name[i] = ' ';
  }
}

void Ge0Parser::ValidateName(string & name)
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

bool Ge0Parser::IsHexChar(char const a)
{
  return ((a >= '0' && a <= '9') || (a >= 'A' && a <= 'F') || (a >= 'a' && a <= 'f'));
}
}  // namespace ge0

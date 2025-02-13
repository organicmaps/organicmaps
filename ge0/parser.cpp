#include "ge0/parser.hpp"

#include "ge0/url_generator.hpp"

#include "coding/url.hpp"

#include "geometry/mercator.hpp"

#include "base/assert.hpp"
#include "base/math.hpp"
#include "base/string_utils.hpp"

#include <algorithm>
#include <sstream>

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

bool Ge0Parser::Parse(std::string const & url, Result & result)
{
  // Original URL format:
  //
  //       +------------------  1 byte: zoom level
  //       |+-------+---------  9 bytes: lat,lon
  //       ||       | +--+----  Variable number of bytes: point name
  //       ||       | |  |
  // om://ZCoordba64/Name

  // Alternative format (differs only in the prefix):
  // http://omaps.app/ZCoordba64/Name

  for (auto prefix : kGe0Prefixes)
  {
    if (url.starts_with(prefix))
      return ParseAfterPrefix(url, prefix.size(), result);
  }

  return false;
}

bool Ge0Parser::ParseAfterPrefix(std::string const & url, size_t from, Result & result)
{
  std::string_view remaining(url.data() + from, url.size() - from);
  size_t posSep = remaining.find_first_of("?#");
  if (posSep != std::string_view::npos)
    remaining = remaining.substr(0, posSep);
  if (remaining.empty())
    return false;
  if (remaining.find(',') != std::string_view::npos)
  {
    std::string_view coords;
    std::string_view name;
    size_t slashPos = remaining.find('/');
    if (slashPos == std::string_view::npos)
    {
      coords = remaining;
    }
    else
    {
      coords = remaining.substr(0, slashPos);
      name = remaining.substr(slashPos + 1);
      while (!name.empty() && name.front() == '/')
        name.remove_prefix(1);
      while (!name.empty() && name.back() == '/')
        name.remove_suffix(1);
    }
    double lat, lon, zoom;
    int pos = 0;
    int count = sscanf(coords.data(), " %lf , %lf %n", &lat, &lon, &pos);
    if (count == 2 && pos == (int)coords.size())
    {
      zoom = 18.0;
    }
    else
    {
      pos = 0;
      count = sscanf(coords.data(), " %lf , %lf , %lf %n", &lat, &lon, &zoom, &pos);
      if (count != 3 || pos != (int)coords.size())
        return false;
    }
    if (lat < -90.0 || lat > 90.0 || lon < -180.0 || lon > 180.0)
      return false;
    if (!std::isfinite(zoom) || zoom < 1.0 || zoom > 22.0)
      return false;
      
    result.m_lat = lat;
    result.m_lon = lon;
    result.m_zoomLevel = zoom;
    
    if (!name.empty())
      result.m_name = DecodeName(std::string(name));
    return true;
  }

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
  result.m_zoomLevel = DecodeZoom(zoomI);

  if (!DecodeLatLon(url.substr(posLatLon, lengthLatLon), result.m_lat, result.m_lon))
    return false;

  ASSERT(mercator::ValidLat(result.m_lat), (result.m_lat));
  ASSERT(mercator::ValidLon(result.m_lon), (result.m_lon));

  if (url.size() >= posName)
  {
    CHECK_GREATER(posName, 0, ());
    if (url[posName - 1] != '/')
      return false;
    result.m_name = DecodeName(url.substr(posName, std::min(url.size() - posName, kMaxNameLength)));
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

bool Ge0Parser::DecodeLatLon(std::string const & s, double & lat, double & lon)
{
  int latInt = 0;
  int lonInt = 0;
  if (!DecodeLatLonToInt(s, latInt, lonInt))
    return false;

  lat = DecodeLatFromInt(latInt, (1 << kMaxCoordBits) - 1);
  lon = DecodeLonFromInt(lonInt, (1 << kMaxCoordBits) - 1);
  return true;
}

bool Ge0Parser::DecodeLatLonToInt(std::string const & s, int & lat, int & lon)
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

std::string Ge0Parser::DecodeName(std::string name)
{
  ValidateName(name);
  name = url::UrlDecode(name);
  SpacesToUnderscore(name);
  return name;
}

void Ge0Parser::SpacesToUnderscore(std::string & name)
{
  for (size_t i = 0; i < name.size(); ++i)
  {
    if (name[i] == ' ')
      name[i] = '_';
    else if (name[i] == '_')
      name[i] = ' ';
  }
}

void Ge0Parser::ValidateName(std::string & name)
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

std::string DebugPrint(Ge0Parser::Result const & r)
{
  std::ostringstream oss;
  oss << "ParseResult [";
  oss << "zoom=" << r.m_zoomLevel << ", ";
  oss << "lat=" << r.m_lat << ", ";
  oss << "lon=" << r.m_lon << ", ";
  oss << "name=" << r.m_name << "]";
  return oss.str();
}
}  // namespace ge0

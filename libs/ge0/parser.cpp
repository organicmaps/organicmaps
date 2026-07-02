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
    if (url.starts_with(prefix))
      return ParseAfterPrefix(url, prefix.size(), result);

  return false;
}

bool Ge0Parser::ParseAfterPrefix(std::string const & url, size_t from, Result & result)
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
  result.m_zoomLevel = DecodeZoom(zoomI);

  if (!DecodeLatLon(std::string_view{url}.substr(posLatLon, lengthLatLon), result.m_lat, result.m_lon))
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

bool Ge0Parser::ParseClearCoordinates(std::string_view path, Result & result)
{
  auto const isDigit = [](char c) { return c >= '0' && c <= '9'; };
  auto const isDecimalCoordinate = [&isDigit](std::string_view s)
  {
    if (s.empty())
      return false;
    if (s.front() == '-')
      s.remove_prefix(1);

    auto const dot = s.find('.');
    if (dot == std::string_view::npos || dot == 0 || dot + 1 == s.size())
      return false;

    return std::all_of(s.begin(), s.begin() + dot, isDigit) && std::all_of(s.begin() + dot + 1, s.end(), isDigit);
  };

  // Exact path form: <lat>,<lon>[/<name>]. The coordinates must keep a decimal
  // point so plain integers and short ge0 codes are not treated as clear-coordinate links.
  auto const comma = path.find(',');
  if (comma == std::string_view::npos)
    return false;

  auto const slash = path.find('/', comma + 1);
  auto const latPart = path.substr(0, comma);
  auto const lonPart =
      slash == std::string_view::npos ? path.substr(comma + 1) : path.substr(comma + 1, slash - comma - 1);
  if (!isDecimalCoordinate(latPart) || !isDecimalCoordinate(lonPart))
    return false;

  double lat, lon;
  if (!strings::to_double(latPart, lat) || !strings::to_double(lonPart, lon))
    return false;
  if (!mercator::ValidLat(lat) || !mercator::ValidLon(lon))
    return false;

  result.m_lat = lat;
  result.m_lon = lon;
  result.m_zoomLevel = 0.0;  // The caller fills zoom from "?z=" (or a default).
  result.m_name = slash == std::string_view::npos ? std::string{} : DecodeName(std::string(path.substr(slash + 1)));
  return true;
}

uint8_t Ge0Parser::DecodeBase64Char(char const c)
{
  return kBase64ReverseCharTable[static_cast<uint8_t>(c)];
}

double Ge0Parser::DecodeZoom(uint8_t const zoomByte)
{
  // Coding zoom -  int newZoom = ((oldZoom - 4) * 4)
  return static_cast<double>(zoomByte) / 4 + 4;
}

bool Ge0Parser::DecodeLatLon(std::string_view s, double & lat, double & lon)
{
  int latInt = 0;
  int lonInt = 0;
  if (!DecodeLatLonToInt(s, latInt, lonInt))
    return false;

  lat = DecodeLatFromInt(latInt, (1 << kMaxCoordBits) - 1);
  lon = DecodeLonFromInt(lonInt, (1 << kMaxCoordBits) - 1);
  return true;
}

bool Ge0Parser::DecodeLatLonToInt(std::string_view s, int & lat, int & lon)
{
  if (s.size() > static_cast<size_t>(kMaxPointBytes))
    return false;

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
  int const remainingBits = 3 * (kMaxPointBytes - static_cast<int>(s.size())) - 1;
  if (remainingBits >= 0)
  {
    int const middleOfSquare = 1 << remainingBits;
    lat += middleOfSquare;
    lon += middleOfSquare;
  }
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
    if (name[i] == ' ')
      name[i] = '_';
    else if (name[i] == '_')
      name[i] = ' ';
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

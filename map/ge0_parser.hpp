#pragma once
#include "base/base.hpp"
#include "std/string.hpp"


namespace url_scheme
{

struct ApiPoint;

class Ge0Parser
{
public:
  Ge0Parser();

  bool Parse(string const & url, url_scheme::ApiPoint & outPoint, double & outZoomLevel);

protected:
  uint8_t DecodeBase64Char(char const c);
  static double DecodeZoom(uint8_t const zoomByte);
  void DecodeLatLon(string const & url, double & lat, double & lon);
  void DecodeLatLonToInt(string const & url, int & lat, int & lon, size_t const bytes);
  double DecodeLatFromInt(int const lat, int const maxValue);
  double DecodeLonFromInt(int const lon, int const maxValue);
  string DecodeName(string name);
  void SpacesToUnderscore(string & name);
  void ValidateName(string & name);
  static bool IsHexChar(char const a);

private:
  uint8_t m_base64ReverseCharTable[256];
};

}  // namespace url_api

#pragma once

#include <cstddef>
#include <cstdint>
#include <string>

namespace ge0
{
class Ge0Parser
{
public:
  Ge0Parser();

  bool Parse(std::string const & url, double & outLat, double & outLon, std::string & outName, double & outZoomLevel);

protected:
  uint8_t DecodeBase64Char(char const c);
  static double DecodeZoom(uint8_t const zoomByte);
  void DecodeLatLon(std::string const & url, double & lat, double & lon);
  void DecodeLatLonToInt(std::string const & url, int & lat, int & lon, size_t const bytes);
  double DecodeLatFromInt(int const lat, int const maxValue);
  double DecodeLonFromInt(int const lon, int const maxValue);
  std::string DecodeName(std::string name);
  void SpacesToUnderscore(std::string & name);
  void ValidateName(std::string & name);
  static bool IsHexChar(char const a);

private:
  uint8_t m_base64ReverseCharTable[256];
};
}  // namespace ge0

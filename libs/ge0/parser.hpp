#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <string>

namespace ge0
{
class Ge0Parser
{
public:
  // Used by map/mwm_url.cpp.
  static constexpr std::array<std::string_view, 6> kGe0Prefixes = {
      {"https://omaps.app/", "om://", "http://omaps.app/", "ge0://", "http://ge0.me/", "https://ge0.me/"}
  };

  struct Result
  {
    double m_zoomLevel = 0.0;
    double m_lat = 0.0;
    double m_lon = 0.0;
    std::string m_name;
  };

  Ge0Parser();

  bool Parse(std::string const & url, Result & result);
  bool ParseAfterPrefix(std::string const & url, size_t from, Result & result);

protected:
  uint8_t DecodeBase64Char(char const c);
  static double DecodeZoom(uint8_t const zoomByte);
  bool DecodeLatLon(std::string const & s, double & lat, double & lon);
  bool DecodeLatLonToInt(std::string const & s, int & lat, int & lon);
  double DecodeLatFromInt(int const lat, int const maxValue);
  double DecodeLonFromInt(int const lon, int const maxValue);
  std::string DecodeName(std::string name);
  void SpacesToUnderscore(std::string & name);
  void ValidateName(std::string & name);
  static bool IsHexChar(char const a);

private:
  uint8_t m_base64ReverseCharTable[256];
};

std::string DebugPrint(Ge0Parser::Result const & r);
}  // namespace ge0

#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>

namespace ge0
{
class Ge0Parser
{
public:
  // Used by map/mwm_url.cpp.
  static constexpr std::array<std::string_view, 6> kGe0Prefixes = {
      {"https://omaps.app/", "om://", "http://omaps.app/", "ge0://", "http://ge0.me/", "https://ge0.me/"}};

  struct Result
  {
    double m_zoomLevel = 0.0;
    double m_lat = 0.0;
    double m_lon = 0.0;
    std::string m_name;
  };

  bool Parse(std::string const & url, Result & result);
  bool ParseAfterPrefix(std::string const & url, size_t from, Result & result);

  // Parses clear decimal coordinates "<lat>,<lon>[/<name>]" from a URL path (scheme, host and
  // query already stripped). Both coordinates must contain a decimal point. Zoom is NOT read here;
  // the caller takes it from the "?z=" query parameter. Fills result.m_lat/m_lon/m_name and leaves
  // m_zoomLevel at 0. Returns false when the path does not exactly match the supported form.
  static bool ParseClearCoordinates(std::string_view path, Result & result);

protected:
  static uint8_t DecodeBase64Char(char c);
  static double DecodeZoom(uint8_t const zoomByte);
  static bool DecodeLatLon(std::string_view s, double & lat, double & lon);
  static bool DecodeLatLonToInt(std::string_view s, int & lat, int & lon);
  static double DecodeLatFromInt(int const lat, int const maxValue);
  static double DecodeLonFromInt(int const lon, int const maxValue);
  static std::string DecodeName(std::string name);
  static void SpacesToUnderscore(std::string & name);
  static void ValidateName(std::string & name);
  static bool IsHexChar(char const a);

private:
  static constexpr std::array<uint8_t, 256> kBase64ReverseCharTable = [] consteval
  {
    std::array<uint8_t, 256> t;
    t.fill(255);
    constexpr char kAlphabet[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_";
    for (uint8_t i = 0; i < 64; ++i)
      t[static_cast<uint8_t>(kAlphabet[i])] = i;
    return t;
  }();
};

std::string DebugPrint(Ge0Parser::Result const & r);
}  // namespace ge0

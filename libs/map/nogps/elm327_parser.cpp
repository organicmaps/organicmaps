#include "map/nogps/elm327_parser.hpp"

#include "base/string_utils.hpp"

#include <cctype>
#include <string>

namespace nogps
{
std::optional<int> ParseElm327Speed(std::string_view response)
{
  // Spaces may be turned off by ATS0, several ECUs may answer on separate lines.
  for (auto const line : strings::Tokenize(response, "\r\n"))
  {
    std::string hex;
    for (char const c : line)
      if (!std::isspace(static_cast<unsigned char>(c)))
        hex += static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
    auto const index = hex.find("410D");
    if (index == std::string::npos || hex.size() < index + 6)
      continue;
    int speed;
    if (strings::to_int(std::string_view(hex).substr(index + 4, 2), speed, 16))
      return speed;
  }
  return {};
}
}  // namespace nogps

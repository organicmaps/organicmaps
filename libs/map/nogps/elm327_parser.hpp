#pragma once

#include <optional>
#include <string_view>

namespace nogps
{
/// Parses an ELM327 response to the OBD-II request "010D" (vehicle speed).
/// \param response everything the adapter has sent before the '>' prompt, e.g. "SEARCHING...\r41 0D 3C\r".
/// \returns the speed in km/h, or nothing if the response doesn't contain it (NO DATA, errors, etc.).
std::optional<int> ParseElm327Speed(std::string_view response);
}  // namespace nogps

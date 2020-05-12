#include "transit/world_feed/color_picker.hpp"

#include "drape_frontend/apply_feature_functors.hpp"
#include "drape_frontend/color_constants.hpp"

#include "drape/color.hpp"

#include "base/string_utils.hpp"

#include <limits>
#include <tuple>

namespace
{
std::tuple<double, double, double> GetColors(dp::Color const & color)
{
  return {color.GetRedF(), color.GetGreenF(), color.GetBlueF()};
}

double GetSquareDistance(dp::Color const & color1, dp::Color const & color2)
{
  auto [r1, g1, b1] = GetColors(color1);
  auto [r2, g2, b2] = GetColors(color2);
  return (r1 - r2) * (r1 - r2) + (g1 - g2) * (g1 - g2) + (b1 - b2) * (b1 - b2);
}
}  // namespace

namespace transit
{
ColorPicker::ColorPicker() { df::LoadTransitColors(); }

std::string ColorPicker::GetNearestColor(std::string const & rgb)
{
  static std::string const kDefaultColor = "default";
  if (rgb.empty())
    return kDefaultColor;

  auto [it, inserted] = m_colorsToNames.emplace(rgb, kDefaultColor);
  if (!inserted)
    return it->second;

  std::string nearestColor = kDefaultColor;

  unsigned int intColor;
  // We do not need to add to the cache invalid color, so we just return.
  if (!strings::to_uint(rgb, intColor, 16))
    return nearestColor;

  dp::Color const color = df::ToDrapeColor(static_cast<uint32_t>(intColor));
  double minDist = std::numeric_limits<double>::max();

  for (auto const & [name, transitColor] : df::GetTransitClearColors())
  {
    if (double const dist = GetSquareDistance(color, transitColor); dist < minDist)
    {
      minDist = dist;
      nearestColor = name;
    }
  }
  if (nearestColor.find(df::kTransitColorPrefix + df::kTransitLinePrefix) == 0)
  {
    nearestColor =
        nearestColor.substr(df::kTransitColorPrefix.size() + df::kTransitLinePrefix.size());
  }

  it->second = nearestColor;
  return nearestColor;
}
}  // namespace transit

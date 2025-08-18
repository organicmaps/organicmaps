#include "transit/world_feed/color_picker.hpp"

#include "drape_frontend/apply_feature_functors.hpp"

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

double GetDistance(dp::Color const & color1, dp::Color const & color2)
{
  auto [r1, g1, b1] = GetColors(color1);
  auto [r2, g2, b2] = GetColors(color2);

  // We use the cmetric (Color metric) for calculating the distance between two colors.
  // https://en.wikipedia.org/wiki/Color_difference
  // It reflects human perception of closest match for a specific colour. The formula weights RGB
  // values to better fit eye perception and performs well at proper determinations of colors
  // contributions, brightness of these colors, and degree to which human vision has less tolerance
  // for these colors.
  double const redMean = (r1 + r2) / 2.0;

  double const redDelta = r1 - r2;
  double const greenDelta = g1 - g2;
  double const blueDelta = b1 - b2;

  return (2.0 + redMean / 256.0) * redDelta * redDelta + 4 * greenDelta * greenDelta +
         (2.0 + (255.0 - redMean) / 256.0) * blueDelta * blueDelta;
}
}  // namespace

namespace transit
{
ColorPicker::ColorPicker()
{
  df::LoadTransitColors();
  // We need only colors for route polylines, not for text. So we skip items like
  // 'transit_text_navy' and work only with items like 'transit_navy'.
  for (auto const & [name, color] : df::GetTransitClearColors())
    if (name.find(df::kTransitTextPrefix) == std::string::npos)
      m_drapeClearColors.emplace(name, color);
}

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

  for (auto const & [name, transitColor] : m_drapeClearColors)
  {
    if (double const dist = GetDistance(color, transitColor); dist < minDist)
    {
      minDist = dist;
      nearestColor = name;
    }
  }

  if (nearestColor.find(df::kTransitColorPrefix + df::kTransitLinePrefix) == 0)
    nearestColor = nearestColor.substr(df::kTransitColorPrefix.size() + df::kTransitLinePrefix.size());

  it->second = nearestColor;
  return nearestColor;
}
}  // namespace transit

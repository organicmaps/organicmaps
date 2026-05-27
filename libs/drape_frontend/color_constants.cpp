#include "drape_frontend/color_constants.hpp"
#include "drape_frontend/apply_feature_functors.hpp"

#include "platform/platform.hpp"

#include "indexer/drawing_rules.hpp"
#include "indexer/map_style_reader.hpp"

#include "coding/reader.hpp"

#include "base/assert.hpp"
#include "base/string_utils.hpp"

#include <glaze/json.hpp>

#include <map>

namespace df
{
namespace transit_colors_json
{
struct TransitColorInfo
{
  std::string clear;
  std::string night;
  std::string text;
};

struct TransitColorsJson
{
  std::map<std::string, TransitColorInfo> colors;
};
}  // namespace transit_colors_json

namespace
{
std::string const kTransitColorFileName = "transit_colors.txt";

class TransitColorsHolder
{
public:
  dp::Color GetColor(std::string_view name) const
  {
    auto const isDarkStyle = MapStyleIsDark(GetStyleReader().GetCurrentStyle());
    auto const & colors = isDarkStyle ? m_nightColors : m_clearColors;
    auto const it = colors.find(name);
    if (it == colors.cend())
      return dp::Color();
    return it->second;
  }

  void Load()
  {
    std::string data;
    try
    {
      ReaderPtr<Reader>(GetPlatform().GetReader(kTransitColorFileName)).ReadAsString(data);
    }
    catch (RootException const & ex)
    {
      LOG(LWARNING, ("Loading transit colors failed:", ex.what()));
      return;
    }

    try
    {
      transit_colors_json::TransitColorsJson transitColors;
      glz::opts constexpr opts{.error_on_unknown_keys = false};
      if (auto const error = glz::read<opts>(transitColors, data); error)
        MYTHROW(RootException, (glz::format_error(error, data)));

      for (auto const & [name, colorInfo] : transitColors.colors)
      {
        m_clearColors[df::GetTransitColorName(name)] = ParseColor(colorInfo.clear);
        m_nightColors[df::GetTransitColorName(name)] = ParseColor(colorInfo.night);
        m_clearColors[df::GetTransitTextColorName(name)] = ParseColor(colorInfo.text);
        m_nightColors[df::GetTransitTextColorName(name)] = ParseColor(colorInfo.text);
      }
    }
    catch (RootException const & e)
    {
      LOG(LWARNING, ("Reading transit colors failed:", e.Msg()));
    }
  }

  ColorsMapT const & GetClearColors() const { return m_clearColors; }

private:
  dp::Color ParseColor(std::string const & colorStr)
  {
    unsigned int color;
    if (strings::to_uint(colorStr, color, 16))
      return df::ToDrapeColor(static_cast<uint32_t>(color));
    LOG(LWARNING, ("Color parsing failed:", colorStr));
    return dp::Color();
  }

  ColorsMapT m_clearColors;
  ColorsMapT m_nightColors;
};

TransitColorsHolder & TransitColors()
{
  static TransitColorsHolder h;
  return h;
}
}  // namespace

std::string GetTransitColorName(ColorConstant const & localName)
{
  return (kTransitColorPrefix + kTransitLinePrefix).append(localName);
}

std::string GetTransitTextColorName(ColorConstant const & localName)
{
  return (kTransitColorPrefix + kTransitTextPrefix).append(localName);
}

bool IsTransitColor(ColorConstant const & constant)
{
  return constant.starts_with(kTransitColorPrefix);
}

dp::Color GetColorConstant(ColorConstant const & constant)
{
  if (IsTransitColor(constant))
    return TransitColors().GetColor(constant);
  uint32_t const color = drule::GetCurrentRules().GetColor(constant);
  return ToDrapeColor(color);
}

ColorsMapT const & GetTransitClearColors()
{
  return TransitColors().GetClearColors();
}

void LoadTransitColors()
{
  TransitColors().Load();
}
}  // namespace df

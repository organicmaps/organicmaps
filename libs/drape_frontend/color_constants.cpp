#include "drape_frontend/color_constants.hpp"
#include "drape_frontend/apply_feature_functors.hpp"

#include "platform/platform.hpp"

#include "indexer/drawing_rules.hpp"
#include "indexer/map_style_reader.hpp"

#include "coding/reader.hpp"

#include "base/assert.hpp"
#include "base/string_utils.hpp"

#include "cppjansson/cppjansson.hpp"

#include <fstream>

namespace
{
std::string const kTransitColorFileName = "transit_colors.txt";

class TransitColorsHolder
{
public:
  dp::Color GetColor(std::string const & name) const
  {
    auto const style = GetStyleReader().GetCurrentStyle();
    auto const isDarkStyle = style == MapStyle::MapStyleDefaultDark || style == MapStyle::MapStyleVehicleDark;
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
      base::Json root(data);

      if (root.get() == nullptr)
        return;

      auto colors = json_object_get(root.get(), "colors");
      if (colors == nullptr)
        return;

      char const * name = nullptr;
      json_t * colorInfo = nullptr;
      json_object_foreach(colors, name, colorInfo)
      {
        ASSERT(name != nullptr, ());
        ASSERT(colorInfo != nullptr, ());

        std::string strValue;
        FromJSONObject(colorInfo, "clear", strValue);
        m_clearColors[df::GetTransitColorName(name)] = ParseColor(strValue);
        FromJSONObject(colorInfo, "night", strValue);
        m_nightColors[df::GetTransitColorName(name)] = ParseColor(strValue);
        FromJSONObject(colorInfo, "text", strValue);
        m_clearColors[df::GetTransitTextColorName(name)] = ParseColor(strValue);
        m_nightColors[df::GetTransitTextColorName(name)] = ParseColor(strValue);
      }
    }
    catch (base::Json::Exception const & e)
    {
      LOG(LWARNING, ("Reading transit colors failed:", e.Msg()));
    }
  }

  std::map<std::string, dp::Color> const & GetClearColors() const { return m_clearColors; }

private:
  dp::Color ParseColor(std::string const & colorStr)
  {
    unsigned int color;
    if (strings::to_uint(colorStr, color, 16))
      return df::ToDrapeColor(static_cast<uint32_t>(color));
    LOG(LWARNING, ("Color parsing failed:", colorStr));
    return dp::Color();
  }

  std::map<std::string, dp::Color> m_clearColors;
  std::map<std::string, dp::Color> m_nightColors;
};

TransitColorsHolder & TransitColors()
{
  static TransitColorsHolder h;
  return h;
}
}  // namespace

namespace df
{
ColorConstant GetTransitColorName(ColorConstant const & localName)
{
  return kTransitColorPrefix + kTransitLinePrefix + localName;
}

ColorConstant GetTransitTextColorName(ColorConstant const & localName)
{
  return kTransitColorPrefix + kTransitTextPrefix + localName;
}

bool IsTransitColor(ColorConstant const & constant)
{
  return constant.starts_with(kTransitColorPrefix);
}

dp::Color GetColorConstant(ColorConstant const & constant)
{
  if (IsTransitColor(constant))
    return TransitColors().GetColor(constant);
  uint32_t const color = drule::rules().GetColor(constant);
  return ToDrapeColor(color);
}

std::map<std::string, dp::Color> const & GetTransitClearColors()
{
  return TransitColors().GetClearColors();
}

void LoadTransitColors()
{
  TransitColors().Load();
}
}  // namespace df

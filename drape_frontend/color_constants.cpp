#include "drape_frontend/color_constants.hpp"
#include "drape_frontend/apply_feature_functors.hpp"

#include "platform/platform.hpp"

#include "indexer/drawing_rules.hpp"
#include "indexer/map_style_reader.hpp"

#include "coding/reader.hpp"

#include "base/assert.hpp"
#include "base/string_utils.hpp"

#include "3party/jansson/myjansson.hpp"

#include <fstream>
#include <map>

using namespace std;

namespace
{
string const kTransitColorFileName = "transit_colors.txt";

class TransitColorsHolder
{
public:
  dp::Color GetColor(string const & name) const
  {
    auto const style = GetStyleReader().GetCurrentStyle();
    auto const isDarkStyle = style == MapStyle::MapStyleDark || style == MapStyle::MapStyleVehicleDark;
    auto const & colors = isDarkStyle ? m_nightColors : m_clearColors;
    auto const it = colors.find(name);
    if (it == colors.cend())
    {
      LOG(LWARNING, ("Requested transit color '" + name + "' is not found"));
      return dp::Color();
    }
    return it->second;
  }

  void Load()
  {
    string data;
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
      my::Json root(data);

      if (root.get() == nullptr)
        return;

      auto colors = json_object_get(root.get(), "colors");
      if (colors == nullptr)
        return;

      const char * name = nullptr;
      json_t * colorInfo = nullptr;
      json_object_foreach(colors, name, colorInfo)
      {
        ASSERT(name != nullptr, ());
        ASSERT(colorInfo != nullptr, ());

        string strValue;
        FromJSONObject(colorInfo, "clear", strValue);
        m_clearColors[df::GetTransitColorName(name)] = ParseColor(strValue);
        FromJSONObject(colorInfo, "night", strValue);
        m_nightColors[df::GetTransitColorName(name)] = ParseColor(strValue);
        FromJSONObject(colorInfo, "text", strValue);
        m_clearColors[df::GetTransitTextColorName(name)] = ParseColor(strValue);
        m_nightColors[df::GetTransitTextColorName(name)] = ParseColor(strValue);
      }
    }
    catch (my::Json::Exception const & e)
    {
      LOG(LWARNING, ("Reading transit colors failed:", e.Msg()));
    }
  }

private:
  dp::Color ParseColor(string const & colorStr)
  {
    unsigned int color;
    if (strings::to_uint(colorStr, color, 16))
      return df::ToDrapeColor(static_cast<uint32_t>(color));
    LOG(LWARNING, ("Color parsing failed:", colorStr));
    return dp::Color();
  }

  map<string, dp::Color> m_clearColors;
  map<string, dp::Color> m_nightColors;
};

TransitColorsHolder & TransitColors()
{
  static TransitColorsHolder h;
  return h;
}
}  // namespace

namespace df
{
string const kTransitColorPrefix = "transit_";
string const kTransitTextPrefix = "text_";
string const kTransitLinePrefix = "line_";

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
  return strings::StartsWith(constant, kTransitColorPrefix);
}

dp::Color GetColorConstant(ColorConstant const & constant)
{
  if (IsTransitColor(constant))
    return TransitColors().GetColor(constant);
  uint32_t const color = drule::rules().GetColor(constant);
  return ToDrapeColor(color);
}

void LoadTransitColors()
{
  TransitColors().Load();
}
}  // namespace df

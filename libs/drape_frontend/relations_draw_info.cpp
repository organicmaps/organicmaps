#include "relations_draw_info.hpp"

#include "drape_frontend/stylist.hpp"

#include "indexer/feature.hpp"

#include "platform/settings.hpp"

#include <algorithm>

namespace df
{
void RelationsDrawSettings::Load(MapStyle style)
{
  hiking = settings::IsEnabled(kHikingEnabledKey);
  cycling = GetMapStyleMode(style) == MapStyleMode::Cycling;
  dark = MapStyleIsDark(style);
}

RelationsDrawSettings RelationsDrawSettings::ForTile(MapStyle style, int zoom)
{
  RelationsDrawSettings settings;
  // Cycling connections are needed as soon as dedicated cycleways are shown at z11.
  if (zoom < 11 || MapStyleIsVehicle(style))
    return settings;
  settings.Load(style);
  settings.hiking = settings.hiking && zoom >= 12;
  return settings;
}

bool RelationsDrawSettings::MatchHiking(feature::RouteRelationBase::Type type) const
{
  using RR = feature::RouteRelationBase;
  return hiking && (type == RR::Type::Foot || type == RR::Type::Hiking);
}

bool RelationsDrawSettings::MatchCycling(feature::RouteRelationBase::Type type) const
{
  using RR = feature::RouteRelationBase;
  return cycling && (type == RR::Type::Bicycle || type == RR::Type::MTB);
}

bool RelationsDrawSettings::MatchHikingOrCycling(feature::RouteRelationBase::Type type) const
{
  return MatchHiking(type) || MatchCycling(type);
}

Stylist MakeLineStylist(FeatureType & f, uint8_t zoom, int8_t deviceLang, MapStyle style,
                        RelationsDrawSettings const & settings)
{
  auto const matchesRoute = [&](uint32_t relID)
  { return settings.MatchHikingOrCycling(f.ReadRelation(relID).GetType()); };
  if (settings.IsEmpty() || std::ranges::none_of(f.GetRelations(), matchesRoute))
    return {f, zoom, deviceLang, style};

  // Cycling keeps the selected style for visible infrastructure and borrows Outdoors for missing connections.
  if (settings.cycling)
  {
    Stylist cycling(f, zoom, deviceLang, style);
    if (!cycling.m_lineRules.empty())
      return cycling;
  }
  return {f, zoom, deviceLang, settings.dark ? MapStyleOutdoorsDark : MapStyleOutdoorsLight};
}

dp::Color constexpr kDefaultRouteColor = dp::Color::Purple();

void RelationsDrawInfo::Init(FeatureType & ft)
{
  if (m_sett.IsEmpty())
    return;

  buffer_vector<std::pair<std::string, int>, 4> refs;
  for (uint32_t relID : ft.GetRelations())
  {
    auto rel = ft.ReadRelation(relID);
    if (m_sett.MatchHikingOrCycling(rel.GetType()) || (m_sett.PT && rel.IsPTRoute()))
    {
      auto clr = rel.GetColor();
      if (clr == kEmptyColor)
        clr = kDefaultRouteColor;

      // Find an existing color to update frequency.
      size_t i = 0;
      size_t const sz = m_colors.size();
      for (; i < sz; ++i)
      {
        if (m_colors[i].first == clr)
        {
          m_colors[i].second++;
          break;
        }
      }
      // Add new color.
      if (i == sz)
        m_colors.push_back({clr, 1});
    }

    if (m_sett.PT)
    {
      auto const & r = rel.GetRel().GetRef();
      if (!r.empty())
      {
        // Get prefix integer value to sort by.
        char * stop;
        long const num = std::strtol(r.c_str(), &stop, 10);
        refs.emplace_back(r, num > 0 ? num : 1000000L);
      }
    }
  }

  if (m_colors.empty())
    return;

  // Keep relation colors readable in the tile's captured theme.
  bool const isLightTheme = !m_sett.dark;
  for (auto & [c, _] : m_colors)
  {
    dp::HSL hsl = dp::Color2HSL(c);
    if (hsl.AdjustLightness(isLightTheme))
      c = dp::HSL2Color(hsl);
  }

  // Most used color first.
  // Use stable sort to keep equal Relations order (less relID).
  /// @todo For the nice order (consistent with the neighbor Ways) they should be sorted somehow by geom criteria
  /// (say from North/West to South/East), independently from the actual Way's direction.
  std::stable_sort(m_colors.begin(), m_colors.end(),
                   [](auto const & r1, auto const & r2) { return r1.second > r2.second; });

  if (m_sett.PT && !refs.empty())
  {
    // Sort and unique "ref" by bus' number.
    std::sort(refs.begin(), refs.end(), [](auto const & r1, auto const & r2)
    {
      if (r1.second == r2.second)
        return r1.first < r2.first;
      return r1.second < r2.second;
    });

    refs.erase(
        std::unique(refs.begin(), refs.end(), [](auto const & r1, auto const & r2) { return r1.first == r2.first; }),
        refs.end());

    // Join into one string.
    for (size_t i = 0; i < refs.size(); ++i)
    {
      if (i > 0)
        m_text += ", ";
      m_text += refs[i].first;
    }
  }
}

dp::Color RelationsDrawInfo::GetTextColor() const
{
  return m_colors.front().first;
}

}  // namespace df

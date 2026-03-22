#include "relations_draw_info.hpp"

#include "indexer/feature.hpp"
#include "indexer/map_style_reader.hpp"

#include "platform/settings.hpp"

namespace df
{
void RelationsDrawSettings::Load()
{
  hiking = settings::IsEnabled(kHikingEnabledKey);
  cycling = settings::IsEnabled(kCyclingEnabledKey);
}

bool RelationsDrawSettings::MatchHikingOrCycling(feature::RouteRelationBase::Type type) const
{
  using RR = feature::RouteRelationBase;
  return ((hiking && (type == RR::Type::Foot || type == RR::Type::Hiking)) ||
          (cycling && (type == RR::Type::Bicycle || type == RR::Type::MTB)));
}

dp::Color constexpr kDefaultRouteColor{128, 0, 128};  // purple

bool RelationsDrawInfo::HasHikingOrCycling(FeatureType & ft) const
{
  for (uint32_t relID : ft.GetRelations())
    if (m_sett.MatchHikingOrCycling(ft.ReadRelationType(relID)))
      return true;
  return false;
}

void RelationsDrawInfo::Init(FeatureType & ft)
{
  using RR = feature::RouteRelationBase;

  buffer_vector<std::pair<std::string, int>, 4> refs;
  for (uint32_t relID : ft.GetRelations())
  {
    auto const rel = ft.ReadRelation<RR>(relID);
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
      auto const & r = rel.GetRef();
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

  // Adjust colors to the current theme for readability.
  bool const isLightTheme = !MapStyleIsDark(GetStyleReader().GetCurrentStyle());
  for (auto & [c, _] : m_colors)
  {
    dp::HSL hsl = dp::Color2HSL(c);
    if (hsl.AdjustLightness(isLightTheme))
      c = dp::HSL2Color(hsl);
  }

  // Most used color first.
  std::sort(m_colors.begin(), m_colors.end(), [](auto const & r1, auto const & r2) { return r1.second > r2.second; });

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

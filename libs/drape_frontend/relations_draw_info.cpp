#include "relations_draw_info.hpp"

#include "indexer/feature.hpp"


namespace df
{

void RelationsDrawInfo::Init(FeatureType & ft)
{
  bool const isHiking = m_style == MapStyleOutdoorsLight || m_style == MapStyleOutdoorsLight;
  /// @todo
  bool const isBicycle = false;
  bool const isPT = false;

  using RR = feature::RouteRelationBase;

  buffer_vector<std::pair<std::string, int>, 4> refs;
  for (uint32_t relID : ft.GetRelations())
  {
    auto const rel = ft.ReadRelation(relID);
    if ((isHiking && (rel.GetType() == RR::Type::Foot || rel.GetType() == RR::Type::Hiking)) ||
        (isBicycle && (rel.GetType() == RR::Type::Bicycle || rel.GetType() == RR::Type::MTB)) ||
        (isPT && (rel.GetType() == RR::Type::Bus || rel.GetType() == RR::Type::Tram ||
                  rel.GetType() == RR::Type::Trolleybus)))
    {
      auto clr = rel.GetColor();
      if (clr == kEmptyColor)
        clr = dp::Color{128, 0, 128}; // purple

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

    if (isPT)
    {
      auto const & r = rel.GetRef();
      if (!r.empty())
      {
        // Get prefix integer value to sort by.
        char * stop;
        refs.push_back({r, std::strtol(r.c_str(), &stop, 10)});
      }
    }
  }

  if (isPT && !refs.empty())
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

    if (m_colors.empty())
      m_colors.push_back({dp::Color::Blue(), 1});

    // Most used color first.
    std::sort(m_colors.begin(), m_colors.end(),
              [](auto const & r1, auto const & r2) { return r1.second > r2.second; });
  }
}

} // namespace df

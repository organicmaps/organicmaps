#pragma once
#include "drape/color.hpp"

#include "indexer/map_style.hpp"
#include "indexer/route_relation.hpp"

#include "base/buffer_vector.hpp"

#include <string>

class FeatureType;

namespace df
{
class RelationsDrawInfo
{
  // color + freq
  buffer_vector<std::pair<dp::Color, int>, 4> m_colors;
  std::string m_text;

  MapStyle m_style;

public:
  static dp::Color constexpr kEmptyColor = feature::RouteRelation::kEmptyColor;

  explicit RelationsDrawInfo(MapStyle style) : m_style(style) {}

  void Init(FeatureType & ft);

  template <class FnT>
  void ForEachColorWithOffset(float width, FnT && fn) const
  {
    int const sz = m_colors.size();
    int const sz2 = sz / 2;
    // sz == 2, offsets:
    // -w/2
    // w/2
    // sz == 3, offsets:
    // -w
    // 0
    // w
    for (int i = 0; i < sz; ++i)
    {
      double pxOffset = (i - sz2) * width;
      if (sz % 2 == 0)
        pxOffset += (width / 2);

      fn(pxOffset, m_colors[i].first);
    }
  }

  dp::Color GetTextColor() const
  {
    auto res = m_colors.front().first;
    if (res == dp::Color::White())
      res = m_colors.size() > 1 ? m_colors[1].first : dp::Color::Blue();
    return res;
  }
};
} // namespace df


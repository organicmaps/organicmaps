#pragma once
#include "drape/color.hpp"

#include "indexer/route_relation.hpp"

#include "base/buffer_vector.hpp"

#include <string>

class FeatureType;

namespace df
{
struct RelationsDrawSettings
{
  bool hiking : 1 = false;
  bool cycling : 1 = false;
  bool PT : 1 = false;
};

class RelationsDrawInfo
{
  // color + freq
  buffer_vector<std::pair<dp::Color, int>, 4> m_colors;
  std::string m_text;

  RelationsDrawSettings m_sett;

public:
  static dp::Color constexpr kEmptyColor = feature::RouteRelation::kEmptyColor;

  explicit RelationsDrawInfo(RelationsDrawSettings const & sett) : m_sett(sett) {}

  bool HasHikingOrCycling(FeatureType & ft) const;
  void Init(FeatureType & ft);

  template <class FnT>
  void ForEachColorWithOffset(float width, FnT && fn) const
  {
    int const sz = m_colors.size();
    int const halfsize = sz / 2;
    bool const isEven = (sz % 2 == 0);
    float const halfWidth = width / 2;

    // sz == 2, offsets:
    // -w/2
    // w/2
    // sz == 3, offsets:
    // -w
    // 0
    // w
    for (int i = 0; i < sz; ++i)
    {
      double pxOffset = (i - halfsize) * width;
      if (isEven)
        pxOffset += halfWidth;

      fn(pxOffset, m_colors[i].first);
    }
  }

  dp::Color GetTextColor() const;
};
}  // namespace df

#pragma once

#include "geometry/point2d.hpp"
#include "geometry/region2d.hpp"

#include <cstdint>
#include <map>
#include <string>
#include <utility>
#include <vector>

namespace indexer
{
// Stores region borders for countries, states, cities, city districts in reverse geocoder.
// Used to check exact object region after getting regions short list from regions geo index.
// Each region may have several outer borders. It may be islands or separated parts like
// Kaliningrad region which is part of Russia or Alaska which is part of US.
// Each outer border may have several inner borders e.g. Vatican and San Marino are
// located inside Italy but are not parts of it.
class Borders
{
public:
  bool IsPointInside(uint64_t id, m2::PointD const & point) const
  {
    auto const range = m_borders.equal_range(id);

    for (auto it = range.first; it != range.second; ++it)
    {
      if (it->second.IsPointInside(point))
        return true;
    }
    return false;
  }

  // Throws Reader::Exception in case of data reading errors.
  void Deserialize(std::string const & filename);

  template <typename BordersVec>
  void DeserializeFromVec(BordersVec const & vec)
  {
    vec.ForEach([this](uint64_t id, std::vector<m2::PointD> const & outer,
                       std::vector<std::vector<m2::PointD>> const & inners) {
      auto it = m_borders.insert(std::make_pair(id, Border()));
      it->second.m_outer = m2::RegionD(outer);
      for (auto const & inner : inners)
        it->second.m_inners.push_back(m2::RegionD(inner));
    });
  }

private:
  struct Border
  {
    Border() = default;

    bool IsPointInside(m2::PointD const & point) const;

    m2::RegionD m_outer;
    std::vector<m2::RegionD> m_inners;
  };

  std::multimap<uint64_t, Border> m_borders;
};
}  // namespace indexer

#pragma once

#include "indexer/city_boundary.hpp"

#include <cstdint>
#include <unordered_map>
#include <utility>
#include <vector>

class Index;

namespace search
{
class CitiesBoundariesTable
{
public:
  class Boundaries
  {
  public:
    Boundaries() = default;

    Boundaries(std::vector<indexer::CityBoundary> const & boundaries, double eps)
      : m_boundaries(boundaries), m_eps(eps)
    {
    }

    Boundaries(std::vector<indexer::CityBoundary> && boundaries, double eps)
      : m_boundaries(std::move(boundaries)), m_eps(eps)
    {
    }

    bool HasPoint(m2::PointD const & p) const;

  private:
    std::vector<indexer::CityBoundary> m_boundaries;
    double m_eps = 0.0;
  };

  bool Load(Index const & index);

  bool Has(uint32_t fid) const { return m_table.find(fid) != m_table.end(); }
  bool Get(uint32_t fid, Boundaries & bs) const;

private:
  std::unordered_map<uint32_t, std::vector<indexer::CityBoundary>> m_table;
  double m_eps = 0.0;
};
}  // namespace search

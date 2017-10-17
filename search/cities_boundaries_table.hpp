#pragma once

#include "indexer/city_boundary.hpp"
#include "indexer/feature_decl.hpp"

#include "geometry/point2d.hpp"

#include <cstdint>
#include <unordered_map>
#include <utility>
#include <vector>

class Index;

namespace feature
{
struct FeatureID;
}

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

    // Returns true iff |p| is inside any of the regions bounded by
    // |*this|.
    bool HasPoint(m2::PointD const & p) const;

  private:
    std::vector<indexer::CityBoundary> m_boundaries;
    double m_eps = 0.0;
  };

  explicit CitiesBoundariesTable(Index const & index) : m_index(index) {}

  bool Load();

  bool Has(FeatureID const & fid) const { return fid.m_mwmId == m_mwmId && Has(fid.m_index); }
  bool Has(uint32_t fid) const { return m_table.find(fid) != m_table.end(); }

  bool Get(FeatureID const & fid, Boundaries & bs) const;
  bool Get(uint32_t fid, Boundaries & bs) const;

private:
  Index const & m_index;
  MwmSet::MwmId m_mwmId;
  std::unordered_map<uint32_t, std::vector<indexer::CityBoundary>> m_table;
  double m_eps = 0.0;
};
}  // namespace search

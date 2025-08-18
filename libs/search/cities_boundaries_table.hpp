#pragma once

#include "indexer/city_boundary.hpp"
#include "indexer/feature_decl.hpp"

#include "geometry/point2d.hpp"
#include "geometry/rect2d.hpp"

#include <string>
#include <unordered_map>
#include <vector>

class DataSource;

namespace search
{
class CitiesBoundariesTable
{
  friend void GetCityBoundariesInRectForTesting(CitiesBoundariesTable const &, m2::RectD const & rect,
                                                std::vector<uint32_t> & featureIds);

public:
  class Boundaries
  {
  public:
    Boundaries() = default;

    Boundaries(std::vector<indexer::CityBoundary> const & boundaries, double eps) : m_boundaries(boundaries), m_eps(eps)
    {}

    // Returns true iff |p| is inside any of the regions bounded by
    // |*this|.
    bool HasPoint(m2::PointD const & p) const;

    m2::RectD GetLimitRect() const
    {
      m2::RectD rect;
      for (auto const & boundary : m_boundaries)
      {
        rect.Add(boundary.m_bbox.Min());
        rect.Add(boundary.m_bbox.Max());
      }
      return rect;
    }

    size_t GetCount() const { return m_boundaries.size(); }

    template <class FnT>
    void ForEachBoundary(FnT && fn) const
    {
      for (size_t i = 0; i < m_boundaries.size(); ++i)
        fn(m_boundaries[i], i);
    }

    friend std::string DebugPrint(Boundaries const & boundaries);

  private:
    std::vector<indexer::CityBoundary> m_boundaries;
    double m_eps = 0.0;
  };

  explicit CitiesBoundariesTable(DataSource const & dataSource) : m_dataSource(dataSource) {}

  bool Load();

  bool Has(FeatureID const & fid) const { return fid.m_mwmId == m_mwmId && Has(fid.m_index); }
  bool Has(uint32_t fid) const { return m_table.find(fid) != m_table.end(); }

  bool Get(FeatureID const & fid, Boundaries & bs) const;
  bool Get(uint32_t fid, Boundaries & bs) const;

  size_t GetSize() const { return m_table.size(); }

private:
  DataSource const & m_dataSource;
  MwmSet::MwmId m_mwmId;
  std::unordered_map<uint32_t, std::vector<indexer::CityBoundary>> m_table;
  double m_eps = 0.0;
};

/// \brief Fills |featureIds| with feature ids of city boundaries if bounding rect of
/// the city boundary crosses |rect|.
/// \note This method is inefficient and is written for debug and test purposes only.
void GetCityBoundariesInRectForTesting(CitiesBoundariesTable const &, m2::RectD const & rect,
                                       std::vector<uint32_t> & featureIds);
}  // namespace search

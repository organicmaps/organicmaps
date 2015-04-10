#pragma once
#include "road_graph.hpp"
#include "vehicle_model.hpp"

#include "../indexer/feature_data.hpp"

#include "../geometry/point2d.hpp"

#include "../base/cache.hpp"

#include "../std/unique_ptr.hpp"
#include "../std/vector.hpp"


class Index;
class FeatureType;

namespace routing
{
class FeaturesRoadGraph : public IRoadGraph
{
  struct CachedFeature
  {
    CachedFeature() : m_speed(0), m_isOneway(false) {}

    buffer_vector<m2::PointD, 32> m_points;
    double m_speed;
    bool m_isOneway;
  };

  // TODO (@gorshenin): ft is not set when feature is not loaded from
  // cache, investigate how to fix this.
  CachedFeature const & GetCachedFeature(uint32_t const ftId, FeatureType & ft, bool fullLoad);

public:
  FeaturesRoadGraph(Index const * pIndex, size_t mwmID);

  // IRoadGraph overrides:
  void GetNearestTurns(RoadPos const & pos, vector<PossibleTurn> & turns) override;
  void ReconstructPath(RoadPosVectorT const & positions, Route & route) override;

  static uint32_t GetStreetReadScale();

  inline size_t GetMwmID() const { return m_mwmID; }

  double GetCacheMiss() const
  {
    if (m_cacheAccess == 0)
      return 0.0;
    return (double)m_cacheMiss / (double)m_cacheAccess;
  }

private:
  friend class CrossFeaturesLoader;

  bool IsOneWay(FeatureType const & ft) const;
  double GetSpeed(FeatureType const & ft) const;
  void LoadFeature(uint32_t id, FeatureType & ft);

  Index const * m_pIndex;
  size_t m_mwmID;
  unique_ptr<IVehicleModel> m_vehicleModel;
  my::Cache<uint32_t, CachedFeature> m_cache;

  uint32_t m_cacheMiss;
  uint32_t m_cacheAccess;
};
} // namespace routing

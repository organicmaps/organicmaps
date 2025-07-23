#pragma once

#include "routing/road_graph.hpp"
#include "routing/route.hpp"
#include "routing/router.hpp"
#include "routing/vehicle_mask.hpp"

#include "routing_common/num_mwm_id.hpp"
#include "routing_common/vehicle_model.hpp"

#include "storage/country_info_getter.hpp"

#include "traffic/traffic_cache.hpp"

#include "indexer/data_source.hpp"

#include "geometry/point2d.hpp"

#include <memory>
#include <set>
#include <string>
#include <vector>

class RoutingTest
{
public:
  RoutingTest(routing::IRoadGraph::Mode mode, routing::VehicleType type, std::set<std::string> const & neededMaps);

  virtual ~RoutingTest() = default;

  void TestRouters(m2::PointD const & startPos, m2::PointD const & finalPos);
  void TestTwoPointsOnFeature(m2::PointD const & startPos, m2::PointD const & finalPos);

protected:
  virtual std::unique_ptr<routing::VehicleModelFactoryInterface> CreateModelFactory() = 0;

  std::unique_ptr<routing::IRouter> CreateRouter(std::string const & name);
  void GetNearestEdges(m2::PointD const & pt,
                       std::vector<std::pair<routing::Edge, geometry::PointWithAltitude>> & edges);

  routing::IRoadGraph::Mode const m_mode;
  routing::VehicleType m_type;
  FrozenDataSource m_dataSource;
  traffic::TrafficCache m_trafficCache;

  std::vector<platform::LocalCountryFile> m_localFiles;
  std::set<std::string> const & m_neededMaps;
  std::shared_ptr<routing::NumMwmIds> m_numMwmIds;
  std::unique_ptr<storage::CountryInfoGetter> m_cig;
};

template <typename Model>
class SimplifiedModelFactory : public routing::VehicleModelFactoryInterface
{
public:
  // Since for test purposes we compare routes lengths to check
  // algorithms consistency, we should use simplified vehicle model,
  // where all available edges have max speed
  class SimplifiedModel : public Model
  {
  public:
    // VehicleModelInterface overrides:
    //
    // SimplifiedModel::GetSpeed() filters features and returns zero
    // speed if feature is not allowed by the base model, or otherwise
    // some speed depending of road type (0 <= speed <= maxSpeed).  For
    // tests purposes for all allowed features speed must be the same as
    // max speed.
    routing::SpeedKMpH GetSpeed(typename Model::FeatureTypes const & types,
                                routing::SpeedParams const & speedParams) const override
    {
      auto const speed = Model::GetSpeed(types, speedParams);
      if (speed.m_weight <= 0.0)
        return routing::SpeedKMpH();

      // Note. Max weight speed is used for eta as well here. It's ok for test purposes.
      return routing::SpeedKMpH(Model::GetMaxWeightSpeed());
    }
  };

  SimplifiedModelFactory() : m_model(std::make_shared<SimplifiedModel>()) {}

  // VehicleModelFactoryInterface overrides:
  std::shared_ptr<routing::VehicleModelInterface> GetVehicleModel() const override { return m_model; }
  std::shared_ptr<routing::VehicleModelInterface> GetVehicleModelForCountry(
      std::string const & /* country */) const override
  {
    return m_model;
  }

private:
  std::shared_ptr<SimplifiedModel> const m_model;
};

void TestRouter(routing::IRouter & router, m2::PointD const & startPos, m2::PointD const & finalPos,
                routing::Route & route);

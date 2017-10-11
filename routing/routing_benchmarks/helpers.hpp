#pragma once

#include "routing/road_graph.hpp"
#include "routing/router.hpp"
#include "routing/road_graph_router.hpp"

#include "routing_common/num_mwm_id.hpp"
#include "routing_common/vehicle_model.hpp"

#include "indexer/index.hpp"

#include "storage/country_info_getter.hpp"

#include "geometry/point2d.hpp"

#include "std/set.hpp"
#include "std/shared_ptr.hpp"
#include "std/string.hpp"
#include "std/unique_ptr.hpp"
#include "std/utility.hpp"
#include "std/vector.hpp"

class RoutingTest
{
public:
  RoutingTest(routing::IRoadGraph::Mode mode, set<string> const & neededMaps);

  virtual ~RoutingTest() = default;

  void TestRouters(m2::PointD const & startPos, m2::PointD const & finalPos);
  void TestTwoPointsOnFeature(m2::PointD const & startPos, m2::PointD const & finalPos);

protected:
  virtual unique_ptr<routing::IDirectionsEngine> CreateDirectionsEngine(
      shared_ptr<routing::NumMwmIds> numMwmIds) = 0;
  virtual unique_ptr<routing::VehicleModelFactoryInterface> CreateModelFactory() = 0;

  template <typename Algorithm>
  unique_ptr<routing::IRouter> CreateRouter(string const & name)
  {
    auto getter = [&](m2::PointD const & pt) { return m_cig->GetRegionCountryId(pt); };
    unique_ptr<routing::IRoutingAlgorithm> algorithm(new Algorithm());
    unique_ptr<routing::IRouter> router(
        new routing::RoadGraphRouter(name, m_index, getter, m_mode, CreateModelFactory(),
                                     move(algorithm), CreateDirectionsEngine(m_numMwmIds)));
    return router;
  }

  void GetNearestEdges(m2::PointD const & pt,
                       vector<pair<routing::Edge, routing::Junction>> & edges);

  routing::IRoadGraph::Mode const m_mode;
  Index m_index;

  shared_ptr<routing::NumMwmIds> m_numMwmIds;
  unique_ptr<storage::CountryInfoGetter> m_cig;
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
    double GetSpeed(FeatureType const & f) const override
    {
      double const speed = Model::GetSpeed(f);
      if (speed <= 0.0)
        return 0.0;
      return Model::GetMaxSpeed();
    }
  };

  SimplifiedModelFactory() : m_model(make_shared<SimplifiedModel>()) {}

  // VehicleModelFactoryInterface overrides:
  shared_ptr<routing::VehicleModelInterface> GetVehicleModel() const override { return m_model; }
  shared_ptr<routing::VehicleModelInterface> GetVehicleModelForCountry(
      string const & /*country*/) const override
  {
    return m_model;
  }

private:
  shared_ptr<SimplifiedModel> const m_model;
};

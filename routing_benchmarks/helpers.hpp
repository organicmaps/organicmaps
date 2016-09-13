#pragma once

#include "routing/road_graph.hpp"
#include "routing/router.hpp"
#include "routing/vehicle_model.hpp"

#include "indexer/index.hpp"

#include "storage/country_info_getter.hpp"

#include "geometry/point2d.hpp"

#include "std/set.hpp"
#include "std/string.hpp"
#include "std/unique_ptr.hpp"
#include "std/utility.hpp"

class RoutingTest
{
public:
  RoutingTest(set<string> const & neededMaps);

  virtual ~RoutingTest() = default;

  void TestRouters(m2::PointD const & startPos, m2::PointD const & finalPos);
  void TestTwoPointsOnFeature(m2::PointD const & startPos, m2::PointD const & finalPos);

protected:
  virtual unique_ptr<routing::IRouter> CreateAStarRouter() = 0;
  virtual unique_ptr<routing::IRouter> CreateAStarBidirectionalRouter() = 0;
  virtual void GetNearestEdges(m2::PointD const & pt,
                               vector<pair<routing::Edge, routing::Junction>> & edges) = 0;

  Index m_index;
  unique_ptr<storage::CountryInfoGetter> m_cig;
};

template <typename Model>
class SimplifiedModelFactory : public routing::IVehicleModelFactory
{
public:
  // Since for test purposes we compare routes lengths to check
  // algorithms consistency, we should use simplified vehicle model,
  // where all available edges have max speed
  class SimplifiedModel : public Model
  {
  public:
    // IVehicleModel overrides:
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

  // IVehicleModelFactory overrides:
  shared_ptr<routing::IVehicleModel> GetVehicleModel() const override { return m_model; }
  shared_ptr<routing::IVehicleModel> GetVehicleModelForCountry(
      string const & /*country*/) const override
  {
    return m_model;
  }

private:
  shared_ptr<SimplifiedModel> const m_model;
};

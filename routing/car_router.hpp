#pragma once

#include "routing/osrm_data_facade.hpp"
#include "routing/osrm_engine.hpp"
#include "routing/route.hpp"
#include "routing/router.hpp"
#include "routing/routing_mapping.hpp"

#include "std/unique_ptr.hpp"
#include "std/vector.hpp"

namespace feature { class TypesHolder; }

class Index;
struct RawRouteData;
struct PhantomNode;
struct PathData;
class FeatureType;

namespace routing
{
class RoadGraphRouter;
struct RoutePathCross;
using TCheckedPath = vector<RoutePathCross>;

typedef vector<FeatureGraphNode> TFeatureGraphNodeVec;

class CarRouter : public IRouter
{
public:
  typedef vector<double> GeomTurnCandidateT;

  CarRouter(Index * index, TCountryFileFn const & countryFileFn,
            unique_ptr<RoadGraphRouter> roadGraphRouter);

  virtual string GetName() const override;

  ResultCode CalculateRoute(m2::PointD const & startPoint, m2::PointD const & startDirection,
                            m2::PointD const & finalPoint, RouterDelegate const & delegate,
                            Route & route) override;

  virtual void ClearState() override;

  /*! Fast checking ability of route construction
   *  @param startPoint starting road point
   *  @param finalPoint final road point
   *  @param countryFileFn function for getting filename from point
   *  @param index mwmSet index
   *  @returns true if we can start routing process with a given data.
   */
  static bool CheckRoutingAbility(m2::PointD const & startPoint, m2::PointD const & finalPoint,
                                  TCountryFileFn const & countryFileFn, Index * index);

protected:
  /*!
   * \brief FindPhantomNodes finds OSRM graph nodes by point and graph name.
   * \param mapName Name of the map without data file extension.
   * \param point Point in lon/lat coordinates.
   * \param direction Movement direction vector in planar coordinates.
   * \param res Result graph nodes.
   * \param maxCount Maximum count of graph nodes in the result vector.
   * \param mapping Reference to routing indexes.
   * \return NoError if there are any nodes in res. RouteNotFound otherwise.
   */
  IRouter::ResultCode FindPhantomNodes(m2::PointD const & point,
                                       m2::PointD const & direction, TFeatureGraphNodeVec & res,
                                       size_t maxCount, TRoutingMappingPtr const & mapping);

private:
  /*!
   * \brief Makes route (points turns and other annotations) from the map cross structs and submits
   * them to @route class
   * \warning monitors m_requestCancel flag for process interrupting.
   * \param path vector of pathes through mwms
   * \param route class to render final route
   * \return NoError or error code
   */
  ResultCode MakeRouteFromCrossesPath(TCheckedPath const & path, RouterDelegate const & delegate,
                                      Route & route);

  bool IsEdgeIndexExisting(Index::MwmId const & mwmId);

  /*!
   * \brief Builds a route within one mwm using A* if edge index section is available and osrm otherwise.
   * Then reconstructs the route and restore all route attributes.
   * \param route The found route is added the the |route| if the method returns true.
   * \return true if route is build and false otherwise.
   */
  // @TODO. The behavior of the method should be changed. This method should check if osrm section
  // available and if so use them. If not, RoadGraphRouter and A* should be used.
  bool FindSingleRouteDispatcher(FeatureGraphNode const & source, FeatureGraphNode const & target,
                                 RouterDelegate const & delegate, TRoutingMappingPtr & mapping,
                                 Route & route);

  /*! Find single shortest path in a single MWM between 2 sets of edges
     * \param source: vector of source edges to make path
     * \param taget: vector of target edges to make path
     * \param facade: OSRM routing data facade to recover graph information
     * \param rawRoutingResult: routing result store
     * \return true when path exists, false otherwise.
     */
  bool FindRouteFromCases(TFeatureGraphNodeVec const & source, TFeatureGraphNodeVec const & target,
                          RouterDelegate const & delegate, TRoutingMappingPtr & mapping, Route & route);

  Index const * m_pIndex;

  TFeatureGraphNodeVec m_cachedTargets;
  m2::PointD m_cachedTargetPoint;

  RoutingIndexManager m_indexManager;

  unique_ptr<RoadGraphRouter> m_roadGraphRouter;
};
}  // namespace routing

#include "generator/routing_index_generator.hpp"

#include "generator/borders_generator.hpp"
#include "generator/borders_loader.hpp"
#include "generator/routing_helpers.hpp"

#include "routing/base/astar_algorithm.hpp"
#include "routing/cross_mwm_connector.hpp"
#include "routing/cross_mwm_connector_serialization.hpp"
#include "routing/cross_mwm_ids.hpp"
#include "routing/index_graph.hpp"
#include "routing/index_graph_loader.hpp"
#include "routing/index_graph_serialization.hpp"
#include "routing/vehicle_mask.hpp"

#include "routing_common/bicycle_model.hpp"
#include "routing_common/car_model.hpp"
#include "routing_common/pedestrian_model.hpp"

#include "transit/transit_graph_data.hpp"
#include "transit/transit_serdes.hpp"

#include "indexer/data_header.hpp"
#include "indexer/feature.hpp"
#include "indexer/feature_processor.hpp"

#include "coding/file_container.hpp"
#include "coding/file_name_utils.hpp"
#include "coding/geometry_coding.hpp"
#include "coding/point_coding.hpp"
#include "coding/reader.hpp"

#include "geometry/point2d.hpp"

#include "base/checked_cast.hpp"
#include "base/geo_object_id.hpp"
#include "base/logging.hpp"
#include "base/timer.hpp"

#include <algorithm>
#include <cstdint>
#include <functional>
#include <map>
#include <memory>
#include <unordered_map>
#include <vector>

using namespace feature;
using namespace platform;
using namespace routing;
using namespace std;
using namespace std::placeholders;

namespace
{
class VehicleMaskBuilder final
{
public:
  VehicleMaskBuilder(string const & country,
                              CountryParentNameGetterFn const & countryParentNameGetterFn)
    : m_pedestrianModel(
          PedestrianModelFactory(countryParentNameGetterFn).GetVehicleModelForCountry(country))
    , m_bicycleModel(
          BicycleModelFactory(countryParentNameGetterFn).GetVehicleModelForCountry(country))
    , m_carModel(CarModelFactory(countryParentNameGetterFn).GetVehicleModelForCountry(country))
  {
    CHECK(m_pedestrianModel, ());
    CHECK(m_bicycleModel, ());
    CHECK(m_carModel, ());
  }

  VehicleMask CalcRoadMask(FeatureType & f) const
  {
    return CalcMask(
        f, [&](VehicleModelInterface const & model, FeatureType & f) { return model.IsRoad(f); });
  }

  VehicleMask CalcOneWayMask(FeatureType & f) const
  {
    return CalcMask(
        f, [&](VehicleModelInterface const & model, FeatureType & f) { return model.IsOneWay(f); });
  }

private:
  template <class Fn>
  VehicleMask CalcMask(FeatureType & f, Fn && fn) const
  {
    VehicleMask mask = 0;
    if (fn(*m_pedestrianModel, f))
      mask |= kPedestrianMask;
    if (fn(*m_bicycleModel, f))
      mask |= kBicycleMask;
    if (fn(*m_carModel, f))
      mask |= kCarMask;

    return mask;
  }

  shared_ptr<VehicleModelInterface> const m_pedestrianModel;
  shared_ptr<VehicleModelInterface> const m_bicycleModel;
  shared_ptr<VehicleModelInterface> const m_carModel;
};

class Processor final
{
public:
  Processor(string const & country, CountryParentNameGetterFn const & countryParentNameGetterFn)
    : m_maskBuilder(country, countryParentNameGetterFn)
  {
  }

  void ProcessAllFeatures(string const & filename)
  {
    feature::ForEachFromDat(filename, bind(&Processor::ProcessFeature, this, _1, _2));
  }

  void BuildGraph(IndexGraph & graph) const
  {
    vector<Joint> joints;
    for (auto const & it : m_posToJoint)
    {
      // Need only connected points (2 or more roads)
      if (it.second.GetSize() >= 2)
        joints.emplace_back(it.second);
    }

    graph.Import(joints);
  }

  unordered_map<uint32_t, VehicleMask> const & GetMasks() const { return m_masks; }

private:
  void ProcessFeature(FeatureType & f, uint32_t id)
  {
    VehicleMask const mask = m_maskBuilder.CalcRoadMask(f);
    if (mask == 0)
      return;

    m_masks[id] = mask;
    f.ParseGeometry(FeatureType::BEST_GEOMETRY);

    for (size_t i = 0; i < f.GetPointsCount(); ++i)
    {
      uint64_t const locationKey = PointToInt64Obsolete(f.GetPoint(i), kPointCoordBits);
      m_posToJoint[locationKey].AddPoint(RoadPoint(id, base::checked_cast<uint32_t>(i)));
    }
  }

  VehicleMaskBuilder const m_maskBuilder;
  unordered_map<uint64_t, Joint> m_posToJoint;
  unordered_map<uint32_t, VehicleMask> m_masks;
};

class DijkstraWrapper final
{
public:
  // AStarAlgorithm types aliases:
  using Vertex = Segment;
  using Edge = SegmentEdge;
  using Weight = RouteWeight;

  explicit DijkstraWrapper(IndexGraph & graph) : m_graph(graph) {}

  void GetOutgoingEdgesList(Vertex const & vertex, vector<Edge> & edges)
  {
    edges.clear();
    m_graph.GetEdgeList(vertex, true /* isOutgoing */, edges);
  }

  void GetIngoingEdgesList(Vertex const & vertex, vector<Edge> & edges)
  {
    edges.clear();
    m_graph.GetEdgeList(vertex, false /* isOutgoing */, edges);
  }

  Weight HeuristicCostEstimate(Vertex const & /* from */, Vertex const & /* to */)
  {
    return GetAStarWeightZero<Weight>();
  }

private:
  IndexGraph & m_graph;
};

// Calculate distance from the starting border point to the transition along the border.
// It could be measured clockwise or counterclockwise, direction doesn't matter.
template <typename CrossMwmId>
double CalcDistanceAlongTheBorders(vector<m2::RegionD> const & borders,
                                   CrossMwmConnectorSerializer::Transition<CrossMwmId> const & transition)
{
  auto distance = GetAStarWeightZero<double>();

  for (m2::RegionD const & region : borders)
  {
    vector<m2::PointD> const & points = region.Data();
    CHECK(!points.empty(), ());
    m2::PointD const * prev = &points.back();

    for (m2::PointD const & curr : points)
    {
      m2::PointD intersection;
      if (m2::RegionD::IsIntersect(transition.GetBackPoint(), transition.GetFrontPoint(), *prev,
                                   curr, intersection))
      {
        distance += prev->Length(intersection);
        return distance;
      }

      distance += prev->Length(curr);
      prev = &curr;
    }
  }

  CHECK(false, ("Intersection not found, feature:", transition.GetFeatureId(), "segment:",
                transition.GetSegmentIdx(), "back:", transition.GetBackPoint(), "front:",
                transition.GetFrontPoint()));
  return distance;
}

/// \brief Fills |transitions| for osm id case. That means |Transition::m_roadMask| for items in
/// |transitions| will be combination of |VehicleType::Pedestrian|, |VehicleType::Bicycle|
/// and |VehicleType::Car|.
void CalcCrossMwmTransitions(
    string const & mwmFile, string const & mappingFile, vector<m2::RegionD> const & borders,
    string const & country, CountryParentNameGetterFn const & countryParentNameGetterFn,
    vector<CrossMwmConnectorSerializer::Transition<base::GeoObjectId>> & transitions)
{
  VehicleMaskBuilder const maskMaker(country, countryParentNameGetterFn);
  map<uint32_t, base::GeoObjectId> featureIdToOsmId;
  CHECK(ParseFeatureIdToOsmIdMapping(mappingFile, featureIdToOsmId),
        ("Can't parse feature id to osm id mapping. File:", mappingFile));

  ForEachFromDat(mwmFile, [&](FeatureType & f, uint32_t featureId) {
    VehicleMask const roadMask = maskMaker.CalcRoadMask(f);
    if (roadMask == 0)
      return;

    f.ParseGeometry(FeatureType::BEST_GEOMETRY);
    size_t const pointsCount = f.GetPointsCount();
    if (pointsCount == 0)
      return;

    auto const it = featureIdToOsmId.find(featureId);
    CHECK(it != featureIdToOsmId.end(), ("Can't find osm id for feature id", featureId));
    auto const osmId = it->second;
    CHECK(osmId.GetType() == base::GeoObjectId::Type::ObsoleteOsmWay, ());

    bool prevPointIn = m2::RegionsContain(borders, f.GetPoint(0));

    for (size_t i = 1; i < pointsCount; ++i)
    {
      bool const currPointIn = m2::RegionsContain(borders, f.GetPoint(i));
      if (currPointIn == prevPointIn)
        continue;

      auto const segmentIdx = base::asserted_cast<uint32_t>(i - 1);
      VehicleMask const oneWayMask = maskMaker.CalcOneWayMask(f);

      transitions.emplace_back(osmId, featureId, segmentIdx, roadMask, oneWayMask, currPointIn,
                               f.GetPoint(i - 1), f.GetPoint(i));

      prevPointIn = currPointIn;
    }
  });
}

/// \brief Fills |transitions| for transit case. That means Transition::m_roadMask for items in
/// |transitions| will be equal to VehicleType::Transit after call of this method.
void CalcCrossMwmTransitions(string const & mwmFile, string const & mappingFile,
                             vector<m2::RegionD> const & borders, string const & country,
                             CountryParentNameGetterFn const & /* countryParentNameGetterFn */,
                             vector<CrossMwmConnectorSerializer::Transition<connector::TransitId>> & transitions)
{
  CHECK(mappingFile.empty(), ());
  try
  {
    FilesContainerR cont(mwmFile);
    if (!cont.IsExist(TRANSIT_FILE_TAG))
    {
      LOG(LINFO, ("Transit cross mwm section is not generated because no transit section in mwm:",
                  mwmFile));
      return;
    }
    auto reader = cont.GetReader(TRANSIT_FILE_TAG);

    transit::GraphData graphData;
    graphData.DeserializeForCrossMwm(*reader.GetPtr());
    auto const & stops = graphData.GetStops();
    auto const & edges = graphData.GetEdges();

    auto const getStopIdPoint = [&stops](transit::StopId stopId) -> m2::PointD const & {
      auto const it = equal_range(stops.cbegin(), stops.cend(), transit::Stop(stopId));
      CHECK_EQUAL(
          distance(it.first, it.second), 1,
          ("A stop with id:", stopId, "is not unique or there's no such item in stops:", stops));
      return it.first->GetPoint();
    };

    // Index |i| is a zero based edge index. This zero based index should be increased with
    // |FakeFeatureIds::kTransitGraphFeaturesStart| by setting it as |featureNumerationOffset| for
    // CrossMwmConnector (see CrossMwmIndexGraph::Deserialize()) and then used in Segment class as
    // feature id in transit case.
    for (size_t i = 0; i < edges.size(); ++i)
    {
      auto const & e = edges[i];
      m2::PointD const & stop1Point = getStopIdPoint(e.GetStop1Id());
      m2::PointD const & stop2Point = getStopIdPoint(e.GetStop2Id());
      bool const stop2In = m2::RegionsContain(borders, stop2Point);
      if (m2::RegionsContain(borders, stop1Point) == stop2In)
        continue;

      // Note. One way mask is set to kTransitMask because all transit edges are one way edges.
      transitions.emplace_back(connector::TransitId(e.GetStop1Id(), e.GetStop2Id(), e.GetLineId()),
                               i /* feature id */, 0 /* segment index */, kTransitMask,
                               kTransitMask /* one way mask */, stop2In /* forward is enter */,
                               stop1Point, stop2Point);
    }
  }
  catch (Reader::OpenException const & e)
  {
    CHECK(false, ("Error while reading", TRANSIT_FILE_TAG, "section.", e.Msg()));
  }
}

/// \brief Fills |transitions| and |connectors| params.
/// \note This method fills only |connections| which are applicable for |CrossMwmId|.
/// For example |VehicleType::Pedestrian|, |VehicleType::Bicycle| and |VehicleType::Car|
/// are applicable for |connector::OsmId|.
/// And |VehicleType::Transit| is applicable for |connector::TransitId|.
template <typename CrossMwmId>
void CalcCrossMwmConnectors(
    string const & path, string const & mwmFile, string const & country,
    CountryParentNameGetterFn const & countryParentNameGetterFn, string const & mappingFile,
    vector<CrossMwmConnectorSerializer::Transition<CrossMwmId>> & transitions,
    CrossMwmConnectorPerVehicleType<CrossMwmId> & connectors)
{
  base::Timer timer;
  string const polyFile = base::JoinPath(path, BORDERS_DIR, country + BORDERS_EXTENSION);
  vector<m2::RegionD> borders;
  osm::LoadBorders(polyFile, borders);

  // Note 1. CalcCrossMwmTransitions() method fills vector |transitions|.
  // There are two implementation of the method for |connector::OsmId| and for |connector::TransitId|.
  // For all items in |transitions| |Transition::m_roadMask| may be set to any combination of masks:
  // GetVehicleMask(VehicleType::Pedestrian), GetVehicleMask(VehicleType::Bicycle) and
  // GetVehicleMask(VehicleType::Car) for |connector::OsmId| implementation.
  // For all items in |transitions| |Transition::m_roadMask| is set to
  // GetVehicleMask(VehicleType::Transit) for |connector::TransitId| implementation.
  // Note 2. Taking into account note 1 it's clear that field |Transition<TransitId>::m_roadMask|
  // is always set to |VehicleType::Transit| and field |Transition<OsmId>::m_roadMask| can't have
  // |VehicleType::Transit| value.
  CalcCrossMwmTransitions(mwmFile, mappingFile, borders, country, countryParentNameGetterFn,
                          transitions);
  LOG(LINFO, ("Transitions finished, transitions:", transitions.size(),
      ", elapsed:", timer.ElapsedSeconds(), "seconds"));

  timer.Reset();
  sort(transitions.begin(), transitions.end(),
       [&](CrossMwmConnectorSerializer::Transition<CrossMwmId> const & lhs,
           CrossMwmConnectorSerializer::Transition<CrossMwmId> const & rhs) {
         return CalcDistanceAlongTheBorders(borders, lhs) <
                CalcDistanceAlongTheBorders(borders, rhs);
       });

  LOG(LINFO, ("Transition sorted in", timer.ElapsedSeconds(), "seconds"));

  for (auto const & transition : transitions)
  {
    for (size_t i = 0; i < connectors.size(); ++i)
    {
      VehicleMask const mask = GetVehicleMask(static_cast<VehicleType>(i));
      CrossMwmConnectorSerializer::AddTransition(transition, mask, connectors[i]);
    }
  }

  for (size_t i = 0; i < connectors.size(); ++i)
  {
    auto const vehicleType = static_cast<VehicleType>(i);
    auto const & connector = connectors[i];
    LOG(LINFO, (vehicleType, "model. Number of enters:", connector.GetEnters().size(),
                "Number of exits:", connector.GetExits().size()));
  }
}

template <typename CrossMwmId>
void FillWeights(string const & path, string const & mwmFile, string const & country,
                 CountryParentNameGetterFn const & countryParentNameGetterFn,
                 bool disableCrossMwmProgress, CrossMwmConnector<CrossMwmId> & connector)
{
  base::Timer timer;

  shared_ptr<VehicleModelInterface> vehicleModel =
      CarModelFactory(countryParentNameGetterFn).GetVehicleModelForCountry(country);
  IndexGraph graph(
      make_shared<Geometry>(GeometryLoader::CreateFromFile(mwmFile, vehicleModel)),
      EdgeEstimator::Create(VehicleType::Car, *vehicleModel, nullptr /* trafficStash */));

  MwmValue mwmValue(LocalCountryFile(path, platform::CountryFile(country), 0 /* version */));
  DeserializeIndexGraph(mwmValue, VehicleType::Car, graph);

  map<Segment, map<Segment, RouteWeight>> weights;
  auto const numEnters = connector.GetEnters().size();
  size_t foundCount = 0;
  size_t notFoundCount = 0;
  for (size_t i = 0; i < numEnters; ++i)
  {
    if (!disableCrossMwmProgress && (i % 10 == 0) && (i != 0))
      LOG(LINFO, ("Building leaps:", i, "/", numEnters, "waves passed"));

    Segment const & enter = connector.GetEnter(i);

    AStarAlgorithm<DijkstraWrapper> astar;
    DijkstraWrapper wrapper(graph);
    AStarAlgorithm<DijkstraWrapper>::Context context;
    astar.PropagateWave(wrapper, enter,
                        [](Segment const & /* vertex */) { return true; } /* visitVertex */,
                        context);

    for (Segment const & exit : connector.GetExits())
    {
      if (context.HasDistance(exit))
      {
        weights[enter][exit] = context.GetDistance(exit);
        ++foundCount;
      }
      else
      {
        ++notFoundCount;
      }
    }
  }

  connector.FillWeights([&](Segment const & enter, Segment const & exit) {
    auto it0 = weights.find(enter);
    if (it0 == weights.end())
      return connector::kNoRoute;

    auto it1 = it0->second.find(exit);
    if (it1 == it0->second.end())
      return connector::kNoRoute;

    return it1->second.ToCrossMwmWeight();
  });

  LOG(LINFO, ("Leaps finished, elapsed:", timer.ElapsedSeconds(), "seconds, routes found:",
              foundCount, ", not found:", notFoundCount));
}

serial::GeometryCodingParams LoadGeometryCodingParams(string const & mwmFile)
{
  DataHeader const dataHeader(mwmFile);
  return dataHeader.GetDefGeometryCodingParams();
}
}  // namespace

namespace routing
{
bool BuildRoutingIndex(string const & filename, string const & country,
                       CountryParentNameGetterFn const & countryParentNameGetterFn)
{
  LOG(LINFO, ("Building routing index for", filename));
  try
  {
    Processor processor(country, countryParentNameGetterFn);
    processor.ProcessAllFeatures(filename);

    IndexGraph graph;
    processor.BuildGraph(graph);

    FilesContainerW cont(filename, FileWriter::OP_WRITE_EXISTING);
    FileWriter writer = cont.GetWriter(ROUTING_FILE_TAG);

    auto const startPos = writer.Pos();
    IndexGraphSerializer::Serialize(graph, processor.GetMasks(), writer);
    auto const sectionSize = writer.Pos() - startPos;

    LOG(LINFO, ("Routing section created:", sectionSize, "bytes,", graph.GetNumRoads(), "roads,",
                graph.GetNumJoints(), "joints,", graph.GetNumPoints(), "points"));
    return true;
  }
  catch (RootException const & e)
  {
    LOG(LERROR, ("An exception happened while creating", ROUTING_FILE_TAG, "section:", e.what()));
    return false;
  }
}

/// \brief Serializes all the cross mwm information to |sectionName| of |mwmFile| including:
/// * header
/// * transitions
/// * weight buffers if any
template <typename CrossMwmId>
void SerializeCrossMwm(string const & mwmFile, string const & sectionName,
                       CrossMwmConnectorPerVehicleType<CrossMwmId> const & connectors,
                       vector<CrossMwmConnectorSerializer::Transition<CrossMwmId>> const & transitions)
{
  serial::GeometryCodingParams const codingParams = LoadGeometryCodingParams(mwmFile);
  FilesContainerW cont(mwmFile, FileWriter::OP_WRITE_EXISTING);
  auto writer = cont.GetWriter(sectionName);
  auto const startPos = writer.Pos();
  CrossMwmConnectorSerializer::Serialize(transitions, connectors, codingParams, writer);
  auto const sectionSize = writer.Pos() - startPos;

  LOG(LINFO, ("Cross mwm section generated, size:", sectionSize, "bytes"));
}

void BuildRoutingCrossMwmSection(string const & path, string const & mwmFile,
                                 string const & country,
                                 CountryParentNameGetterFn const & countryParentNameGetterFn,
                                 string const & osmToFeatureFile, bool disableCrossMwmProgress)
{
  LOG(LINFO, ("Building cross mwm section for", country));
  using CrossMwmId = base::GeoObjectId;
  CrossMwmConnectorPerVehicleType<CrossMwmId> connectors;
  vector<CrossMwmConnectorSerializer::Transition<CrossMwmId>> transitions;

  CalcCrossMwmConnectors(path, mwmFile, country, countryParentNameGetterFn, osmToFeatureFile,
                         transitions, connectors);

  // We use leaps for cars only. To use leaps for other vehicle types add weights generation
  // here and change WorldGraph mode selection rule in IndexRouter::CalculateSubroute.
  FillWeights(path, mwmFile, country, countryParentNameGetterFn, disableCrossMwmProgress,
              connectors[static_cast<size_t>(VehicleType::Car)]);

  CHECK(connectors[static_cast<size_t>(VehicleType::Transit)].IsEmpty(), ());
  SerializeCrossMwm(mwmFile, CROSS_MWM_FILE_TAG, connectors, transitions);
}

void BuildTransitCrossMwmSection(string const & path, string const & mwmFile,
                                 string const & country,
                                 CountryParentNameGetterFn const & countryParentNameGetterFn)
{
  LOG(LINFO, ("Building transit cross mwm section for", country));
  using CrossMwmId = connector::TransitId;
  CrossMwmConnectorPerVehicleType<CrossMwmId> connectors;
  vector<CrossMwmConnectorSerializer::Transition<CrossMwmId>> transitions;

  CalcCrossMwmConnectors(path, mwmFile, country, countryParentNameGetterFn, "" /* mapping file */,
                         transitions, connectors);

  CHECK(connectors[static_cast<size_t>(VehicleType::Pedestrian)].IsEmpty(), ());
  CHECK(connectors[static_cast<size_t>(VehicleType::Bicycle)].IsEmpty(), ());
  CHECK(connectors[static_cast<size_t>(VehicleType::Car)].IsEmpty(), ());
  SerializeCrossMwm(mwmFile, TRANSIT_CROSS_MWM_FILE_TAG, connectors, transitions);
}
}  // namespace routing

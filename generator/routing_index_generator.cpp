#include "generator/routing_index_generator.hpp"
#include "generator/borders_generator.hpp"
#include "generator/borders_loader.hpp"

#include "routing/index_graph.hpp"
#include "routing/index_graph_serialization.hpp"
#include "routing/vehicle_mask.hpp"

#include "routing/cross_mwm_ramp.hpp"
#include "routing/cross_mwm_ramp_serialization.hpp"
#include "routing_common/bicycle_model.hpp"
#include "routing_common/car_model.hpp"
#include "routing_common/pedestrian_model.hpp"

#include "indexer/feature.hpp"
#include "indexer/feature_processor.hpp"
#include "indexer/point_to_int64.hpp"

#include "coding/file_container.hpp"
#include "coding/file_name_utils.hpp"

#include "base/checked_cast.hpp"
#include "base/logging.hpp"

#include "std/bind.hpp"
#include "std/shared_ptr.hpp"
#include "std/unordered_map.hpp"
#include "std/vector.hpp"

using namespace feature;
using namespace platform;
using namespace routing;

namespace
{
class VehicleMaskMaker final
{
public:
  explicit VehicleMaskMaker(string const & country)
    : m_pedestrianModel(PedestrianModelFactory().GetVehicleModelForCountry(country))
    , m_bicycleModel(BicycleModelFactory().GetVehicleModelForCountry(country))
    , m_carModel(CarModelFactory().GetVehicleModelForCountry(country))
  {
    CHECK(m_pedestrianModel, ());
    CHECK(m_bicycleModel, ());
    CHECK(m_carModel, ());
  }

  VehicleMask CalcRoadMask(FeatureType const & f) const
  {
    VehicleMask mask = 0;
    if (m_pedestrianModel->IsRoad(f))
      mask |= kPedestrianMask;
    if (m_bicycleModel->IsRoad(f))
      mask |= kBicycleMask;
    if (m_carModel->IsRoad(f))
      mask |= kCarMask;

    return mask;
  }

  VehicleMask CalcOneWayMask(FeatureType const & f) const
  {
    VehicleMask mask = 0;
    if (m_pedestrianModel->IsOneWay(f))
      mask |= kPedestrianMask;
    if (m_bicycleModel->IsOneWay(f))
      mask |= kBicycleMask;
    if (m_carModel->IsOneWay(f))
      mask |= kCarMask;

    return mask;
  }

private:
  shared_ptr<IVehicleModel> const m_pedestrianModel;
  shared_ptr<IVehicleModel> const m_bicycleModel;
  shared_ptr<IVehicleModel> const m_carModel;
};

class Processor final
{
public:
  explicit Processor(string const & country) : m_maskMaker(country) {}

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
  void ProcessFeature(FeatureType const & f, uint32_t id)
  {
    VehicleMask const mask = m_maskMaker.CalcRoadMask(f);
    if (mask == 0)
      return;

    m_masks[id] = mask;
    f.ParseGeometry(FeatureType::BEST_GEOMETRY);

    for (size_t i = 0; i < f.GetPointsCount(); ++i)
    {
      uint64_t const locationKey = PointToInt64(f.GetPoint(i), POINT_COORD_BITS);
      m_posToJoint[locationKey].AddPoint(RoadPoint(id, base::checked_cast<uint32_t>(i)));
    }
  }

  VehicleMaskMaker const m_maskMaker;
  unordered_map<uint64_t, Joint> m_posToJoint;
  unordered_map<uint32_t, VehicleMask> m_masks;
};

bool BordersContains(vector<m2::RegionD> const & borders, m2::PointD const & point)
{
  for (m2::RegionD const & region : borders)
  {
    if (region.Contains(point))
      return true;
  }

  return false;
}

void CalcCrossMwmTransitions(string const & path, string const & mwmFile, string const & country,
                             vector<CrossMwmRampSerializer::Transition> & transitions,
                             vector<CrossMwmRamp> & ramps)
{
  string const polyFile = my::JoinFoldersToPath({path, BORDERS_DIR}, country + BORDERS_EXTENSION);
  vector<m2::RegionD> borders;
  osm::LoadBorders(polyFile, borders);

  VehicleMaskMaker const maskMaker(country);

  feature::ForEachFromDat(mwmFile, [&](FeatureType const & f, uint32_t featureId) {
    VehicleMask const roadMask = maskMaker.CalcRoadMask(f);
    if (roadMask == 0)
      return;

    f.ParseGeometry(FeatureType::BEST_GEOMETRY);
    size_t const pointsCount = f.GetPointsCount();
    if (pointsCount <= 0)
      return;

    bool prevPointIn = BordersContains(borders, f.GetPoint(0));

    for (size_t i = 1; i < pointsCount; ++i)
    {
      bool const pointIn = BordersContains(borders, f.GetPoint(i));
      if (pointIn != prevPointIn)
      {
        uint32_t const segmentIdx = base::asserted_cast<uint32_t>(i - 1);
        VehicleMask const oneWayMask = maskMaker.CalcOneWayMask(f);

        transitions.emplace_back(featureId, segmentIdx, roadMask, oneWayMask, pointIn,
                                 f.GetPoint(i - 1), f.GetPoint(i));

        for (size_t j = 0; j < ramps.size(); ++j)
        {
          VehicleMask const mask = GetVehicleMask(static_cast<VehicleType>(j));
          CrossMwmRampSerializer::AddTransition(transitions.back(), mask, ramps[j]);
        }
      }

      prevPointIn = pointIn;
    }
  });
}

void FillWeights(string const & path, string const & country, CrossMwmRamp & ramp)
{
  shared_ptr<IVehicleModel> vehicleModel = CarModelFactory().GetVehicleModelForCountry(country);
  shared_ptr<EdgeEstimator> estimator =
      EdgeEstimator::CreateForCar(nullptr /* trafficStash */, vehicleModel->GetMaxSpeed());

  Index index;
  platform::CountryFile countryFile(country);
  index.RegisterMap(LocalCountryFile(path, countryFile, 0));
  MwmSet::MwmHandle handle = index.GetMwmHandleByCountryFile(countryFile);
  CHECK(handle.IsAlive(), ());

  Geometry geometry(GeometryLoader::Create(index, handle.GetId(), vehicleModel));

  ramp.FillWeights([&](Segment const & enter, Segment const & exit) {
    return estimator->CalcHeuristic(geometry.GetPoint(enter.GetRoadPoint(true)),
                                    geometry.GetPoint(exit.GetRoadPoint(true)));
  });
}
}  // namespace

namespace routing
{
bool BuildRoutingIndex(string const & filename, string const & country)
{
  LOG(LINFO, ("Building routing index for", filename));
  try
  {
    Processor processor(country);
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

void BuildCrossMwmSection(string const & path, string const & mwmFile, string const & country)
{
  LOG(LINFO, ("Building cross mwm section for", country));
  my::Timer timer;

  vector<CrossMwmRamp> ramps(static_cast<size_t>(VehicleType::Count), kFakeNumMwmId);

  vector<CrossMwmRampSerializer::Transition> transitions;
  CalcCrossMwmTransitions(path, mwmFile, country, transitions, ramps);

  FillWeights(path, country, ramps[static_cast<size_t>(VehicleType::Car)]);

  FilesContainerW cont(mwmFile, FileWriter::OP_WRITE_EXISTING);
  FileWriter writer = cont.GetWriter(CROSS_MWM_FILE_TAG);

  DataHeader const dataHeader(mwmFile);
  serial::CodingParams const & codingParams = dataHeader.GetDefCodingParams();

  auto const startPos = writer.Pos();
  CrossMwmRampSerializer::Serialize(transitions, ramps, codingParams, writer);
  auto const sectionSize = writer.Pos() - startPos;

  LOG(LINFO, ("Cross mwm section for", country, "generated in", timer.ElapsedSeconds(),
              "seconds, section size:", sectionSize, "bytes, transitions:", transitions.size()));
}
}  // namespace routing

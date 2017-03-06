#include "generator/routing_index_generator.hpp"

#include "routing/index_graph.hpp"
#include "routing/index_graph_serialization.hpp"
#include "routing/vehicle_mask.hpp"

#include "routing_common/bicycle_model.hpp"
#include "routing_common/car_model.hpp"
#include "routing_common/pedestrian_model.hpp"

#include "indexer/feature.hpp"
#include "indexer/feature_processor.hpp"
#include "indexer/point_to_int64.hpp"

#include "coding/file_container.hpp"

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
class Processor final
{
public:
  explicit Processor(string const & country)
    : m_pedestrianModel(PedestrianModelFactory().GetVehicleModelForCountry(country))
    , m_bicycleModel(BicycleModelFactory().GetVehicleModelForCountry(country))
    , m_carModel(CarModelFactory().GetVehicleModelForCountry(country))
  {
    CHECK(m_pedestrianModel, ());
    CHECK(m_bicycleModel, ());
    CHECK(m_carModel, ());
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
  void ProcessFeature(FeatureType const & f, uint32_t id)
  {
    VehicleMask const mask = CalcVehicleMask(f);
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

  VehicleMask CalcVehicleMask(FeatureType const & f) const
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

  shared_ptr<IVehicleModel> const m_pedestrianModel;
  shared_ptr<IVehicleModel> const m_bicycleModel;
  shared_ptr<IVehicleModel> const m_carModel;
  unordered_map<uint64_t, Joint> m_posToJoint;
  unordered_map<uint32_t, VehicleMask> m_masks;
};
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
}  // namespace routing

#include "generator/routing_index_generator.hpp"

#include "routing/bicycle_model.hpp"
#include "routing/car_model.hpp"
#include "routing/index_graph.hpp"
#include "routing/index_graph_serializer.hpp"
#include "routing/pedestrian_model.hpp"

#include "indexer/feature.hpp"
#include "indexer/feature_processor.hpp"
#include "indexer/point_to_int64.hpp"

#include "coding/file_container.hpp"

#include "base/logging.hpp"

#include "std/bind.hpp"
#include "std/shared_ptr.hpp"
#include "std/unordered_map.hpp"
#include "std/unordered_set.hpp"
#include "std/vector.hpp"

using namespace feature;
using namespace platform;
using namespace routing;

namespace
{
class Processor final
{
public:
  Processor(string const & country)
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

  unordered_set<uint32_t> const & GetCarFeatureIds() const { return m_carFeatureIds; }

private:
  void ProcessFeature(FeatureType const & f, uint32_t id)
  {
    if (!IsRoad(f))
      return;

    if (m_carModel->IsRoad(f))
      m_carFeatureIds.insert(id);

    f.ParseGeometry(FeatureType::BEST_GEOMETRY);

    for (size_t i = 0; i < f.GetPointsCount(); ++i)
    {
      uint64_t const locationKey = PointToInt64(f.GetPoint(i), POINT_COORD_BITS);
      m_posToJoint[locationKey].AddPoint(RoadPoint(id, i));
    }
  }

  bool IsRoad(FeatureType const & f) const
  {
    return m_pedestrianModel->IsRoad(f) || m_bicycleModel->IsRoad(f) || m_carModel->IsRoad(f);
  }

  shared_ptr<IVehicleModel> m_pedestrianModel;
  shared_ptr<IVehicleModel> m_bicycleModel;
  shared_ptr<IVehicleModel> m_carModel;
  unordered_map<uint64_t, Joint> m_posToJoint;
  unordered_set<uint32_t> m_carFeatureIds;
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
    IndexGraphSerializer::Serialize(graph, processor.GetCarFeatureIds(), writer);
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

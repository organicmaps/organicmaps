#include "generator/routing_index_generator.hpp"

#include "routing/index_graph.hpp"
#include "routing/routing_helpers.hpp"

#include "indexer/feature.hpp"
#include "indexer/feature_processor.hpp"
#include "indexer/point_to_int64.hpp"
#include "indexer/routing_section.hpp"

#include "coding/file_container.hpp"

#include "base/logging.hpp"

#include "std/bind.hpp"
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

private:
  void ProcessFeature(FeatureType const & f, uint32_t id)
  {
    if (!IsRoad(feature::TypesHolder(f)))
      return;

    f.ParseGeometry(FeatureType::BEST_GEOMETRY);

    for (size_t i = 0; i < f.GetPointsCount(); ++i)
    {
      uint64_t const locationKey = PointToInt64(f.GetPoint(i), POINT_COORD_BITS);
      m_posToJoint[locationKey].AddPoint(RoadPoint(id, i));
    }
  }

  unordered_map<uint64_t, Joint> m_posToJoint;
};
}  // namespace

namespace routing
{
bool BuildRoutingIndex(string const & filename)
{
  LOG(LINFO, ("Building routing index for", filename));
  try
  {
    Processor processor;
    processor.ProcessAllFeatures(filename);

    IndexGraph graph;
    processor.BuildGraph(graph);
    LOG(LINFO, ("Routing index contains", graph.GetNumRoads(), "roads,", graph.GetNumJoints(),
                "joints,", graph.GetNumPoints(), "points"));

    FilesContainerW cont(filename, FileWriter::OP_WRITE_EXISTING);
    FileWriter writer = cont.GetWriter(ROUTING_FILE_TAG);

    RoutingSectionHeader const routingHeader;
    routingHeader.Serialize(writer);
    graph.Serialize(writer);
    return true;
  }
  catch (RootException const & e)
  {
    LOG(LERROR, ("An exception happened while creating", ROUTING_FILE_TAG, "section:", e.what()));
    return false;
  }
}
}  // namespace routing

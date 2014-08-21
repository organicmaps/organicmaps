#include "routing_generator.hpp"

#include "../indexer/index.hpp"
#include "../indexer/classificator_loader.hpp"
#include "../indexer/feature.hpp"
#include "../indexer/ftypes_matcher.hpp"

#include "../3party/osrm/osrm-backend/DataStructures/QueryEdge.h"
#include "../3party/osrm/osrm-backend/Server/DataStructures/InternalDataFacade.h"

namespace routing
{

uint32_t const MAX_SCALE = 15;  // max scale for roads

struct Node2Feature
{
  NodeID m_nodeId;
  FeatureID m_FID;
  uint32_t m_start;
  uint32_t m_len;

  Node2Feature(NodeID nodeId, FeatureID fid, uint32_t start, uint32_t len)
    : m_nodeId(nodeId), m_FID(fid), m_start(start), m_len(len)
  {
  }
};

struct Node2FeatureSetComp
{
  bool operator() (Node2Feature const & a, Node2Feature const & b)
  {
    return a.m_nodeId < b.m_nodeId;
  }
};
typedef std::set<Node2Feature, Node2FeatureSetComp> Node2FeatureSet;

void GenerateNodesInfo(string const & mwmName, string const & osrmName)
{
  classificator::Load();

  Index index;
  m2::RectD rect;
  if (!index.Add(mwmName, rect))
  {
    LOG(LERROR, ("MWM file not found"));
    return;
  }

  ServerPaths server_paths;
  server_paths["hsgrdata"] = boost::filesystem::path(osrmName + ".hsgr");
  server_paths["ramindex"] = boost::filesystem::path(osrmName + ".ramIndex");
  server_paths["fileindex"] = boost::filesystem::path(osrmName + ".fileIndex");
  server_paths["geometries"] = boost::filesystem::path(osrmName + ".geometry");
  server_paths["nodesdata"] = boost::filesystem::path(osrmName + ".nodes");
  server_paths["edgesdata"] = boost::filesystem::path(osrmName + ".edges");
  server_paths["namesdata"] = boost::filesystem::path(osrmName + ".names");
  server_paths["timestamp"] = boost::filesystem::path(osrmName + ".timestamp");

  InternalDataFacade<QueryEdge::EdgeData> facade(server_paths);

  Node2FeatureSet nodesSet;
  uint32_t processed = 0;
  auto fn = [&](FeatureType const & ft)
  {
    processed++;

    if (processed % 1000 == 0)
    {
      if (processed % 10000 == 0)
        std::cout << "[" << processed / 1000 << "k]";
      else
        std::cout << ".";
      std::cout.flush();
    }

    feature::TypesHolder types(ft);
    if (types.GetGeoType() != feature::GEOM_LINE)
      return;

    if (!ftypes::IsStreetChecker::Instance()(ft))
        return;

    std::cout << "------------ FID: " << ft.GetID().m_offset << std::endl;

    ft.ParseGeometry(FeatureType::BEST_GEOMETRY);
    uint32_t lastId = std::numeric_limits<uint32_t>::max();
    uint32_t start = 0;
    for (uint32_t i = 1; i < ft.GetPointsCount(); ++i)
    {
      m2::PointD p = ft.GetPoint(i -1).mid(ft.GetPoint(i));
      FixedPointCoordinate fp(static_cast<int>(p.x * COORDINATE_PRECISION),
                              static_cast<int>(p.y * COORDINATE_PRECISION));

      PhantomNode node;
      if (facade.FindPhantomNodeForCoordinate(fp, node, 18))
      {
        if (node.forward_node_id != lastId)
        {
          if (lastId != std::numeric_limits<uint32_t>::max())
          {
            bool added = nodesSet.insert(Node2Feature(node.forward_node_id, ft.GetID(), start, i - 1)).second;
            assert(added);
          }
          lastId = node.forward_node_id;
          start = i - 1;
          std::cout << "LastID: " << lastId << " ForwardID: " << node.forward_node_id << " Offset: " << node.forward_offset << std::endl;
        }
      } else
        break;
    }
  };
  index.ForEachInRect(fn, MercatorBounds::FullRect(), MAX_SCALE);

  std::cout << "Nodes: " << facade.GetNumberOfNodes() << " Set: " << nodesSet.size() << std::endl;

}

}
